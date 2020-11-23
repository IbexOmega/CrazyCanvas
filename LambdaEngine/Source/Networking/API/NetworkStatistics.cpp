#include "Networking/API/NetworkStatistics.h"

#include "Engine/EngineLoop.h"

#include "Math/Random.h"

namespace LambdaEngine
{
	NetworkStatistics::NetworkStatistics()
	{
		Reset();
	}

	NetworkStatistics::~NetworkStatistics()
	{

	}

	uint32 NetworkStatistics::GetPacketsSent() const
	{
		return m_PacketsSent;
	}

	uint32 NetworkStatistics::GetSegmentsRegistered() const
	{
		return m_SegmentsRegistered;
	}

	uint32 NetworkStatistics::GetSegmentsSent() const
	{
		return m_SegmentsSent;
	}

	uint32 NetworkStatistics::GetReliableSegmentsSent() const
	{
		return m_ReliableSegmentsSent;
	}

	uint32 NetworkStatistics::GetPacketsReceived() const
	{
		return m_PacketsReceived;
	}

	uint32 NetworkStatistics::GetSegmentsReceived() const
	{
		return m_SegmentsReceived;
	}

	uint32 NetworkStatistics::GetReceivingPacketLoss() const
	{
		return m_PacketsLostReceiving;
	}

	uint32 NetworkStatistics::GetSendingPacketLoss() const
	{
		return m_PacketsLostSending;
	}

	float32 NetworkStatistics::GetReceivingPacketLossRate() const
	{
		if (m_PacketsLostReceiving == 0)
			return 0.0;
		return m_PacketsLostReceiving / (float32)m_LastReceivedSequenceNr;
	}

	float32 NetworkStatistics::GetSendingPacketLossRate() const
	{
		if (m_PacketsLostSending == 0)
			return 0.0;
		return m_PacketsLostSending / (float32)m_PacketsSent;
	}

	uint32 NetworkStatistics::GetBytesSent() const
	{
		return m_BytesSent;
	}

	uint32 NetworkStatistics::GetBytesReceived() const
	{
		return m_BytesReceived;
	}

	float64 NetworkStatistics::GetPing() const
	{
		return m_Ping;
	}

	uint64 NetworkStatistics::GetSalt() const
	{
		return m_Salt;
	}

	uint64 NetworkStatistics::GetRemoteSalt() const
	{
		return m_SaltRemote;
	}

	uint32 NetworkStatistics::GetLastReceivedSequenceNr() const
	{
		return m_LastReceivedSequenceNr;
	}

	uint64 NetworkStatistics::GetReceivedSequenceBits() const
	{
		return m_ReceivedSequenceBits;
	}

	uint32 NetworkStatistics::GetLastReceivedAckNr() const
	{
		return m_LastReceivedAckNr;
	}

	uint64 NetworkStatistics::GetReceivedAckBits() const
	{
		return m_ReceivedAckBits;
	}

	uint32 NetworkStatistics::GetLastReceivedReliableUID() const
	{
		return m_LastReceivedReliableUID;
	}

	uint32 NetworkStatistics::GetSegmentsResent() const
	{
		return m_SegmentsResent;
	}

	const THashTable<uint16, uint32>& NetworkStatistics::BeginGetSentSegmentTypeCountTable()
	{
		m_LockPacketTypeSendCounter.lock();
		return m_PacketTypeSendCounter;
	}

	const THashTable<uint16, uint32>& NetworkStatistics::BeginGetReceivedSegmentTypeCountTable()
	{
		m_LockPacketTypeReceiveCounter.lock();
		return m_PacketTypeReceiveCounter;
	}

	void NetworkStatistics::EndGetSentSegmentTypeCountTable()
	{
		m_LockPacketTypeSendCounter.unlock();
	}

	void NetworkStatistics::EndGetReceivedSegmentTypeCountTable()
	{
		m_LockPacketTypeReceiveCounter.unlock();
	}

	CCBuffer<uint16, 60>& NetworkStatistics::BeginGetBytesSentHistory()
	{
		m_LockBytesSentHistory.lock();
		return m_BytesSentHistory;
	}

	CCBuffer<uint16, 60>& NetworkStatistics::BeginGetBytesReceivedHistory()
	{
		m_LockBytesReceivedHistory.lock();
		return m_BytesReceivedHistory;
	}

	void NetworkStatistics::EndGetBytesSentHistory()
	{
		m_LockBytesSentHistory.unlock();
	}

	void NetworkStatistics::EndGetBytesReceivedHistory()
	{
		m_LockBytesReceivedHistory.unlock();
	}

	Timestamp NetworkStatistics::GetTimestampLastSent() const
	{
		return m_TimestampLastSent;
	}

	Timestamp NetworkStatistics::GetTimestampLastReceived() const
	{
		return m_TimestampLastReceived;
	}

	void NetworkStatistics::Reset()
	{
		m_Salt						= Random::UInt64();
		m_SaltRemote				= 0;
		m_Ping						= 20.0f;
		m_PacketsSent				= 0;
		m_SegmentsRegistered		= 0;
		m_SegmentsSent				= 0;
		m_ReliableSegmentsSent		= 0;
		m_PacketsReceived			= 0;
		m_SegmentsReceived			= 0;
		m_BytesSent					= 0;
		m_BytesReceived				= 0;
		m_LastReceivedSequenceNr	= 0;
		m_ReceivedSequenceBits		= 0;
		m_LastReceivedAckNr			= 0;
		m_ReceivedAckBits			= 0;
		m_LastReceivedReliableUID	= 0;
		m_TimestampLastSent			= EngineLoop::GetTimeSinceStart();
		m_TimestampLastReceived		= EngineLoop::GetTimeSinceStart();
		m_SegmentsResent			= 0;
		m_PacketsLostReceiving		= 0;
		m_PacketsLostSending		= 0;

		std::scoped_lock<SpinLock> lock1(m_LockPacketTypeSendCounter);
		std::scoped_lock<SpinLock> lock2(m_LockPacketTypeReceiveCounter);
		m_PacketTypeSendCounter.clear();
		m_PacketTypeReceiveCounter.clear();

		std::scoped_lock<SpinLock> lock3(m_LockBytesSentHistory);
		std::scoped_lock<SpinLock> lock4(m_LockBytesReceivedHistory);
		m_BytesSentHistory.Clear();
		m_BytesReceivedHistory.Clear();
	}

	uint32 NetworkStatistics::RegisterPacketSent()
	{
		m_TimestampLastSent = EngineLoop::GetTimeSinceStart();
		return ++m_PacketsSent;
	}

	uint32 NetworkStatistics::RegisterUniqueSegment(uint16 type)
	{
#ifndef LAMBDA_PRODUCTION
		std::scoped_lock<SpinLock> lock(m_LockPacketTypeSendCounter);
		m_PacketTypeSendCounter[type]++;
#endif

		return ++m_SegmentsRegistered;
	}

	void NetworkStatistics::RegisterUniqueSegmentReceived(uint16 type)
	{
#ifndef LAMBDA_PRODUCTION
		std::scoped_lock<SpinLock> lock(m_LockPacketTypeReceiveCounter);
		m_PacketTypeReceiveCounter[type]++;
#endif
	}

	void NetworkStatistics::RegisterSegmentSent(uint32 segments)
	{
		m_SegmentsSent += segments;
	}

	uint32 NetworkStatistics::RegisterReliableSegmentSent()
	{
		return ++m_ReliableSegmentsSent;
	}

	void NetworkStatistics::RegisterPacketReceived(uint32 segments, uint32 bytes)
	{
		m_PacketsReceived++;
		m_SegmentsReceived += segments,
		m_BytesReceived += bytes;
		m_TimestampLastReceived = EngineLoop::GetTimeSinceStart();

#ifndef LAMBDA_PRODUCTION
		std::scoped_lock<SpinLock> lock(m_LockBytesReceivedHistory);
		m_BytesReceivedHistory.Write((uint16)bytes);
#endif
	}

	void NetworkStatistics::RegisterReliableSegmentReceived()
	{
		m_LastReceivedReliableUID++;
	}

	void NetworkStatistics::RegisterBytesSent(uint32 bytes)
	{
		m_BytesSent += bytes;

#ifndef LAMBDA_PRODUCTION
		std::scoped_lock<SpinLock> lock(m_LockBytesSentHistory);
		m_BytesSentHistory.Write((uint16)bytes);
#endif
	}

	void NetworkStatistics::SetLastReceivedSequenceNr(uint32 sequence)
	{
		m_LastReceivedSequenceNr = sequence;
	}

	void NetworkStatistics::SetReceivedSequenceBits(uint64 sequenceBits)
	{
		m_ReceivedSequenceBits = sequenceBits;
	}

	void NetworkStatistics::SetLastReceivedAckNr(uint32 ack)
	{
		ASSERT(ack > m_LastReceivedAckNr);
		m_LastReceivedAckNr = ack;
	}

	void NetworkStatistics::SetReceivedAckBits(uint64 ackBits)
	{
		m_ReceivedAckBits = ackBits;
	}

	void NetworkStatistics::RegisterReceivingPacketLoss(uint32 packets)
	{
		m_PacketsLostReceiving += packets;
	}

	void NetworkStatistics::RegisterSendingPacketLoss(uint32 packets)
	{
		m_PacketsLostSending += packets;
	}

	void NetworkStatistics::RegisterSegmentResent()
	{
		m_SegmentsResent++;
	}

	void NetworkStatistics::SetRemoteSalt(uint64 salt)
	{
		m_SaltRemote = salt;
	}
}