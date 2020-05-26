#include "Networking/API/PacketTransceiver.h"
#include "Networking/API/ISocketUDP.h"
#include "Networking/API/NetworkStatistics.h"

#include "Math/Random.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	PacketTransceiver::PacketTransceiver() : 
		m_pSocket(nullptr),
		m_BytesReceived(0),
		m_ReceivingLossRatio(0.0f),
		m_TransmittingLossRatio(0.0f),
		m_pSendBuffer(),
		m_pReceiveBuffer()
	{

	}

	PacketTransceiver::~PacketTransceiver()
	{

	}

	int32 PacketTransceiver::Transmit(PacketPool* pPacketPool, std::queue<NetworkPacket*>& packets, std::set<uint32>& reliableUIDsSent, const IPEndPoint& ipEndPoint, NetworkStatistics* pStatistics)
	{
		if (packets.empty())
			return 0;

		PacketTranscoder::Header header;
		uint16 bytesWritten = 0;
		int32 bytesTransmitted = 0;

		header.Sequence = pStatistics->RegisterPacketSent();
		header.Salt		= pStatistics->GetSalt();
		header.Ack		= pStatistics->GetLastReceivedSequenceNr();
		header.AckBits	= pStatistics->GetReceivedSequenceBits();

		PacketTranscoder::EncodePackets(m_pSendBuffer, MAXIMUM_PACKET_SIZE + sizeof(PacketTranscoder::Header), pPacketPool, packets, reliableUIDsSent, bytesWritten, &header);

		pStatistics->RegisterBytesSent(bytesWritten);

#ifndef LAMBDA_CONFIG_PRODUCTION
		if (m_TransmittingLossRatio > 0.0f && Random::Float32() <= m_TransmittingLossRatio)
		{
			LOG_WARNING("[PacketTransceiver]: Simulated Transmitting Packetloss");
			return header.Sequence;
		}
#endif

		if (!m_pSocket->SendTo(m_pSendBuffer, bytesWritten, bytesTransmitted, ipEndPoint))
			return -1;
		else if (bytesWritten != bytesTransmitted)
			return -1;

		return header.Sequence;
	}

	bool PacketTransceiver::ReceiveBegin(IPEndPoint& sender)
	{
		m_BytesReceived = 0;

		if (!m_pSocket->ReceiveFrom(m_pReceiveBuffer, UINT16_MAX, m_BytesReceived, sender))
			return false;

#ifndef LAMBDA_CONFIG_PRODUCTION
		if (m_ReceivingLossRatio > 0.0f && Random::Float32() <= m_ReceivingLossRatio)
		{
			LOG_WARNING("[PacketTransceiver]: Simulated Receiving Packetloss");
			return false;
		}	
#endif

		return m_BytesReceived > 0;
	}

	bool PacketTransceiver::ReceiveEnd(PacketPool* pPacketPool, std::vector<NetworkPacket*>& packets, std::vector<uint32>& newAcks, NetworkStatistics* pStatistics)
	{
		PacketTranscoder::Header header;
		if (!PacketTranscoder::DecodePackets(m_pReceiveBuffer, (uint16)m_BytesReceived, pPacketPool, packets, &header))
			return false;

		if (!ValidateHeaderSalt(&header, pStatistics))
			return false;

		ProcessSequence(header.Sequence, pStatistics);
		ProcessAcks(header.Ack, header.AckBits, pStatistics, newAcks);

		pStatistics->RegisterPacketReceived((uint32)packets.size(), m_BytesReceived);

		return true;
	}

	void PacketTransceiver::SetSocket(ISocketUDP* pSocket)
	{
		m_pSocket = pSocket;
	}

	void PacketTransceiver::SetSimulateReceivingPacketLoss(float32 lossRatio)
	{
		m_ReceivingLossRatio = lossRatio;
	}

	void PacketTransceiver::SetSimulateTransmittingPacketLoss(float32 lossRatio)
	{
		m_TransmittingLossRatio = lossRatio;
	}

	bool PacketTransceiver::ValidateHeaderSalt(PacketTranscoder::Header* header, NetworkStatistics* pStatistics)
	{
		if (header->Salt == 0)
		{
			LOG_ERROR("[PacketTranscoder]: Received a packet without a salt");
			return false;
		}
		else if (pStatistics->GetRemoteSalt() != header->Salt)
		{
			if (pStatistics->GetRemoteSalt() == 0)
			{
				pStatistics->SetRemoteSalt(header->Salt);
				return true;
			}
			else
			{
				LOG_ERROR("[PacketTranscoder]: Received a packet with a new salt [Prev %lu : New %lu]", pStatistics->GetRemoteSalt(), header->Salt);
				return false;
			}
		}
		return true;
	}

	/*
	* Updates the last Received Sequence number and corresponding bits.
	*/
	void PacketTransceiver::ProcessSequence(uint32 sequence, NetworkStatistics* pStatistics)
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

	void PacketTransceiver::ProcessAcks(uint32 ack, uint32 ackBits, NetworkStatistics* pStatistics, std::vector<uint32>& newAcks)
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
				newAcks.push_back(ack - i);
			}
			acks <<= 1;
		}
		
		if (ack > lastReceivedAck)
			newAcks.push_back(ack);
	}
}