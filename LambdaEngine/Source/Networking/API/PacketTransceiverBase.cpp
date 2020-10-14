#include "Networking/API/PacketTransceiverBase.h"
#include "Networking/API/NetworkStatistics.h"

#include "Networking/API/UDP/ISocketUDP.h"

#include "Math/Random.h"

#include "Log/Log.h"

namespace LambdaEngine
{
	PacketTransceiverBase::PacketTransceiverBase() :
		m_BytesReceived(0),
		m_pSendBuffer(),
		m_pReceiveBuffer(),
		m_IgnoreSaltMissmatch(false)
	{

	}

	int32 PacketTransceiverBase::Transmit(SegmentPool* pSegmentPool, std::queue<NetworkSegment*>& segments, std::set<uint32>& reliableUIDsSent, const IPEndPoint& ipEndPoint, NetworkStatistics* pStatistics)
	{
		if (segments.empty())
			return 0;

		PacketTranscoder::Header header;
		uint16 bytesWritten = 0;
		int32 bytesTransmitted = 0;

		header.Sequence = pStatistics->RegisterPacketSent();
		header.Salt = pStatistics->GetSalt();
		header.Ack = pStatistics->GetLastReceivedSequenceNr();
		header.AckBits = pStatistics->GetReceivedSequenceBits();

		PacketTranscoder::EncodeSegments(m_pSendBuffer, MAXIMUM_SEGMENT_SIZE, pSegmentPool, segments, reliableUIDsSent, bytesWritten, &header);

		pStatistics->RegisterBytesSent(bytesWritten);

		if (!TransmitData(m_pSendBuffer, bytesWritten, bytesTransmitted, ipEndPoint))
			return -1;
		else if (bytesWritten != bytesTransmitted)
			return -1;

		pStatistics->RegisterSegmentSent(header.Segments);

		return header.Sequence;
	}

	bool PacketTransceiverBase::ReceiveBegin(IPEndPoint& sender)
	{
		m_BytesReceived = 0;

		if (!ReceiveData(m_pReceiveBuffer, UINT16_MAX, m_BytesReceived, sender))
			return false;

		return m_BytesReceived > 0;
	}

	bool PacketTransceiverBase::ReceiveEnd(SegmentPool* pSegmentPool, TArray<NetworkSegment*>& segments, TArray<uint32>& newAcks, NetworkStatistics* pStatistics)
	{
		PacketTranscoder::Header header;
		if (!PacketTranscoder::DecodeSegments(m_pReceiveBuffer, (uint16)m_BytesReceived, pSegmentPool, segments, &header))
			return false;

		if(!m_IgnoreSaltMissmatch)
			if (!ValidateHeaderSalt(&header, pStatistics))
				return false;

		OnReceiveEnd(&header, newAcks, pStatistics);

		pStatistics->RegisterPacketReceived((uint32)segments.GetSize(), m_BytesReceived);

		return true;
	}

	void PacketTransceiverBase::SetIgnoreSaltMissmatch(bool ignore)
	{
		m_IgnoreSaltMissmatch = ignore;
	}

	bool PacketTransceiverBase::ValidateHeaderSalt(PacketTranscoder::Header* header, NetworkStatistics* pStatistics)
	{
		if (header->Salt == 0)
		{
			LOG_ERROR("[PacketTransceiverBase]: Received a packet without a salt");
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
				LOG_ERROR("[PacketTransceiverBase]: Received a packet with a new salt [Prev %lu : New %lu]", pStatistics->GetRemoteSalt(), header->Salt);
				return false;
			}
		}
		return true;
	}
}