#include "Networking/API/NetworkStatistics.h"

#include "Networking/API/UDP/ISocketUDP.h"
#include "Networking/API/UDP/PacketTransceiverUDP.h"

#include "Math/Random.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	PacketTransceiverUDP::PacketTransceiverUDP() :
		m_pSocket(nullptr),
		m_ReceivingLossRatio(0.0f),
		m_TransmittingLossRatio(0.0f)
	{

	}

	PacketTransceiverUDP::~PacketTransceiverUDP()
	{

	}

	bool PacketTransceiverUDP::Transmit(const uint8* pBuffer, uint32 bytesToSend, int32& bytesSent, const IPEndPoint& ipEndPoint)
	{
		#ifndef LAMBDA_CONFIG_PRODUCTION
		if (m_TransmittingLossRatio > 0.0f && Random::Float32() <= m_TransmittingLossRatio)
		{
			LOG_WARNING("[PacketTransceiverBase]: Simulated Transmitting Packetloss");
			bytesSent = bytesToSend;
			return true;
		}
		#endif

		return m_pSocket->SendTo(pBuffer, bytesToSend, bytesSent, ipEndPoint);
	}

	bool PacketTransceiverUDP::Receive(uint8* pBuffer, uint32 size, int32& bytesReceived, IPEndPoint& pIPEndPoint)
	{
		if (!m_pSocket->ReceiveFrom(pBuffer, size, bytesReceived, pIPEndPoint))
			return false;

#ifndef LAMBDA_CONFIG_PRODUCTION
		if (m_ReceivingLossRatio > 0.0f && Random::Float32() <= m_ReceivingLossRatio)
		{
			LOG_WARNING("[PacketTransceiverBase]: Simulated Receiving Packetloss");
			return false;
		}
#endif
		return true;
	}

	void PacketTransceiverUDP::OnReceiveEnd(PacketTranscoder::Header* pHeader, TArray<uint32>& newAcks, NetworkStatistics* pStatistics)
	{
		ProcessSequence(pHeader->Sequence, pStatistics);
		ProcessAcks(pHeader->Ack, pHeader->AckBits, pStatistics, newAcks);
	}

	void PacketTransceiverUDP::SetSocket(ISocket* pSocket)
	{
		m_pSocket = (ISocketUDP*)pSocket;
	}

	void PacketTransceiverUDP::SetSimulateReceivingPacketLoss(float32 lossRatio)
	{
		m_ReceivingLossRatio = lossRatio;
	}

	void PacketTransceiverUDP::SetSimulateTransmittingPacketLoss(float32 lossRatio)
	{
		m_TransmittingLossRatio = lossRatio;
	}

	/*
	* Updates the last Received Sequence number and corresponding bits.
	*/
	void PacketTransceiverUDP::ProcessSequence(uint32 sequence, NetworkStatistics* pStatistics)
	{
		uint32 lastReceivedSequence = pStatistics->GetLastReceivedSequenceNr();
		if (sequence > lastReceivedSequence)
		{
			//New sequence number received so shift everything delta steps
			int32 delta = sequence - lastReceivedSequence;
			pStatistics->SetLastReceivedSequenceNr(sequence);

			for (int i = 0; i < delta; i++)
			{
				uint32 sequenceBits = pStatistics->GetReceivedSequenceBits();
				if (sequenceBits >> (sizeof(uint32) * 8 - 1) & 0)
				{
					//The last bit that was removed was never acked.
					pStatistics->m_PacketsLost++;
				}
				pStatistics->SetReceivedSequenceBits(sequenceBits << 1);
			}
		}
		else if(sequence < lastReceivedSequence)
		{
			//Old sequence number received so write 1 to the coresponding bit
			int32 index = lastReceivedSequence - sequence - 1;
			if (index >= 0 && index < 32)
			{
				pStatistics->SetReceivedSequenceBits(pStatistics->GetReceivedSequenceBits() | (1 << index));
			}
		}
	}

	void PacketTransceiverUDP::ProcessAcks(uint32 ack, uint32 ackBits, NetworkStatistics* pStatistics, TArray<uint32>& newAcks)
	{
		uint32 lastReceivedAck = pStatistics->GetLastReceivedAckNr();
		uint32 currentAckBits = pStatistics->GetReceivedAckBits();
		uint32 maxValue = std::min(32, (int32)ack);
		uint32 bits = 0;
		uint32 shift = 33 - maxValue;
		ackBits &= shift == 32 ? 0 : (UINT32_MAX >> shift);

		if (ack > lastReceivedAck)
		{
			pStatistics->SetLastReceivedAckNr(ack);

			if (lastReceivedAck > 0)
			{
				currentAckBits <<= 1;
				currentAckBits |= 1;
				currentAckBits <<= (ack - lastReceivedAck - 1);
			}

			bits = currentAckBits;
		}
		else if (ack < lastReceivedAck)
		{
			ackBits <<= 1;
			ackBits |= 1;
			ackBits <<= (lastReceivedAck - ack - 1);

			bits = ackBits | pStatistics->GetReceivedAckBits();
		}

		pStatistics->SetReceivedAckBits(currentAckBits | ackBits);

		uint32 acks = (pStatistics->GetReceivedAckBits() ^ bits);

		for (int i = 32; i > 0; i--)
		{
			if (acks >> (sizeof(uint32) * 8 - 1) & 1)
			{
				newAcks.PushBack(ack - i);
			}
			acks <<= 1;
		}
		
		if (ack > lastReceivedAck)
			newAcks.PushBack(ack);
	}
}