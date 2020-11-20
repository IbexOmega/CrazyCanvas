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

	bool PacketTransceiverUDP::TransmitData(const uint8* pBuffer, uint32 bytesToSend, int32& bytesSent, const IPEndPoint& ipEndPoint)
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

	bool PacketTransceiverUDP::ReceiveData(uint8* pBuffer, uint32 size, int32& bytesReceived, IPEndPoint& pIPEndPoint)
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

	void PacketTransceiverUDP::OnReceiveEnd(PacketTranscoder::Header* pHeader, TSet<uint32>& newAcks, NetworkStatistics* pStatistics)
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
			pStatistics->SetLastReceivedSequenceNr(sequence);

			if (lastReceivedSequence > 0)
			{
				uint64 sequenceBits = pStatistics->GetReceivedSequenceBits();

				//Shift 1, we need to insert GetLastReceivedSequenceNr(), it is not in the bitmask
				sequenceBits <<= 1;
				//Or 1, represents GetLastReceivedSequenceNr()
				sequenceBits |= 1;
				//Shift the rest of the way to align with the new ack timeline (ackBits)
				sequenceBits <<= (sequence - lastReceivedSequence - 1);

				pStatistics->SetReceivedSequenceBits(sequenceBits);
			}
		}
		else if(sequence < lastReceivedSequence)
		{
			//Old sequence number received so write 1 to the coresponding bit
			int64 index = lastReceivedSequence - sequence - 1;
			if (index < 64)
			{
				pStatistics->SetReceivedSequenceBits(pStatistics->GetReceivedSequenceBits() | (1ULL << index));
			}
		}
		else
		{
			LOG_ERROR("[PacketTransceiverUDP]: Latest SEQ already set to received SEQ [%lu]", sequence);
		}
	}

	void PacketTransceiverUDP::ProcessAcks(uint32 ack, uint64 ackBits, NetworkStatistics* pStatistics, TSet<uint32>& newAcks)
	{
		uint64 lastReceivedAck = pStatistics->GetLastReceivedAckNr();
		uint64 currentAckBits = pStatistics->GetReceivedAckBits();
		uint64 resultingAckBits = 0ULL;

		LOG_INFO("[PacketTransceiverUDP]: Last Received Ack [%llu], New Ack [%lu]", lastReceivedAck, ack);

		if (ack > lastReceivedAck)
		{
			pStatistics->SetLastReceivedAckNr(ack);
			newAcks.insert(ack);

			//Check if larger than 0, first ack shouldn't set the last bit because the last bit represents the last previously acked packet (GetLastReceivedAckNr())
			if (lastReceivedAck > 0)
			{
				//Shift 1, we need to insert GetLastReceivedAckNr(), it is not in the bitmask
				currentAckBits <<= 1;
				//Or 1, represents GetLastReceivedAckNr()
				currentAckBits |= 1;
				//Shift the rest of the way to align with the new ack timeline (ackBits)
				currentAckBits <<= (ack - lastReceivedAck - 1);
			}

			//Or with ackBits to set other acked Bits besides ack
			resultingAckBits = currentAckBits | ackBits;

			LOG_INFO("[PacketTransceiverUDP]: ACK is Newer [%lu], Ackbits: %llx", ack, ackBits);
		}
		else if (ack < lastReceivedAck)
		{
			//No Timeline update required for our timeline since this ack is older than our GetLastReceivedAckNr()
			resultingAckBits = currentAckBits;
			//Calculate which bit to set in currentAckBits
			uint64 deltaAck = lastReceivedAck - ack - 1;
			//Set the bit that represents ack in our timeline
			resultingAckBits |= (1ULL << deltaAck);
			//Set the bits in ackBits shifted to our timeline
			resultingAckBits |= (ackBits << deltaAck);
			//Calculate the acks that we might shift off due to the above calculation
			uint64 trashedAckBits = (ackBits >> (64ULL - deltaAck));

			uint64 lastTrashedAck = ack - (64ULL - deltaAck + 1ULL);

			//Assume trashed acks are new acks, add them to newAcks
			for (uint64 i = 0; i < deltaAck; i++)
			{
				if (trashedAckBits & (1ULL << i))
				{
					uint64 trashedAck = lastTrashedAck - i;
					LOG_INFO("[PacketTransceiverUDP]: Trashed Ack [%lu]", trashedAck);
					newAcks.insert((uint32)trashedAck);
				}
			}

			LOG_INFO("[PacketTransceiverUDP]: ACK is Older [%lu], Ackbits: %llx", ack, ackBits);
		}
		else
		{
			LOG_ERROR("[PacketTransceiverUDP]: Latest ACK already set to received ACK [%lu]", ack);
			return;
		}
		
		uint64 newAckBits = resultingAckBits ^ currentAckBits;

		for (uint64 i = 0; i < 64; i++)
		{
			if (newAckBits & (1ULL << i))
			{
				uint64 newAck = ack - (i + 1ULL);
				newAcks.insert((uint32)newAck);
			}
		}

		pStatistics->SetReceivedAckBits(resultingAckBits);

		for (uint32 newAck : newAcks)
		{
			LOG_INFO("[PacketTransceiverUDP]: New Ack [%lu]", newAck);
		}
	}
}