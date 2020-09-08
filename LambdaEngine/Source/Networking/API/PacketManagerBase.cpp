#include "Networking/API/PacketManagerBase.h"

#include "Networking/API/NetworkSegment.h"
#include "Networking/API/IPacketListener.h"
#include "Networking/API/PacketTransceiverBase.h"

#include "Engine/EngineLoop.h"

namespace LambdaEngine
{
	PacketManagerBase::PacketManagerBase(const PacketManagerDesc& desc) :
		m_SegmentPool(desc.PoolSize),
		m_QueueIndex(0)
	{

	}

	uint32 PacketManagerBase::EnqueueSegmentReliable(NetworkSegment* pSegment, IPacketListener* pListener)
	{
		std::scoped_lock<SpinLock> lock(m_LockSegmentsToSend);
		uint32 UID = EnqueueSegment(pSegment, m_Statistics.RegisterReliableSegmentSent());
		m_SegmentsWaitingForAck.insert({ UID, SegmentInfo{ pSegment, pListener, EngineLoop::GetTimeSinceStart()} });
		return UID;
	}

	uint32 PacketManagerBase::EnqueueSegmentUnreliable(NetworkSegment* pSegment)
	{
		std::scoped_lock<SpinLock> lock(m_LockSegmentsToSend);
		return EnqueueSegment(pSegment, 0);
	}

	uint32 PacketManagerBase::EnqueueSegment(NetworkSegment* pSegment, uint32 reliableUID)
	{
		pSegment->GetHeader().UID = m_Statistics.RegisterSegmentSent();
		pSegment->GetHeader().ReliableUID = reliableUID;
		m_SegmentsToSend[m_QueueIndex].push(pSegment);
		return pSegment->GetHeader().UID;
	}

	void PacketManagerBase::Flush(PacketTransceiverBase* pTransceiver)
	{
		int32 indexToUse = m_QueueIndex;
		m_QueueIndex = (m_QueueIndex + 1) % 2;
		std::queue<NetworkSegment*>& segments = m_SegmentsToSend[indexToUse];

		Timestamp timestamp = EngineLoop::GetTimeSinceStart();

		while (!segments.empty())
		{
			Bundle bundle;
			uint32 bundleUID = pTransceiver->Transmit(&m_SegmentPool, segments, bundle.ReliableUIDs, m_IPEndPoint, &m_Statistics);

			if (!bundle.ReliableUIDs.empty())
			{
				bundle.Timestamp = timestamp;

				std::scoped_lock<SpinLock> lock(m_LockBundles);
				m_Bundles.insert({ bundleUID, bundle });
			}
		}
	}

	void PacketManagerBase::QueryEnd(TArray<NetworkSegment*>& segmentsReceived)
	{
		m_SegmentPool.FreeSegments(segmentsReceived);
	}

	void PacketManagerBase::DeleteOldBundles()
	{
		// Maybe add a callback for disconnection?
		Timestamp maxAllowedTime = m_Statistics.GetPing() * 100;
		Timestamp currentTime = EngineLoop::GetTimeSinceStart();

		TArray<uint32> bundlesToDelete;

		std::scoped_lock<SpinLock> lock(m_LockBundles);
		for (auto& pair : m_Bundles)
		{
			if (currentTime - pair.second.Timestamp > maxAllowedTime)
			{
				bundlesToDelete.PushBack(pair.first);
				m_Statistics.RegisterPacketLoss();
			}
		}

		for (uint32 UID : bundlesToDelete)
			m_Bundles.erase(UID);
	}

	void PacketManagerBase::Tick(Timestamp delta)
	{
		static const Timestamp delay = Timestamp::Seconds(1);

		m_Timer += delta;
		if (m_Timer >= delay)
		{
			m_Timer -= delay;
			DeleteOldBundles();
		}
	}

	SegmentPool* PacketManagerBase::GetSegmentPool()
	{
		return &m_SegmentPool;
	}

	const NetworkStatistics* PacketManagerBase::GetStatistics() const
	{
		return &m_Statistics;
	}

	const IPEndPoint& PacketManagerBase::GetEndPoint() const
	{
		return m_IPEndPoint;
	}

	void PacketManagerBase::SetEndPoint(const IPEndPoint& ipEndPoint)
	{
		m_IPEndPoint = ipEndPoint;
	}

	void PacketManagerBase::Reset()
	{
		std::scoped_lock<SpinLock> lock1(m_LockSegmentsToSend);
		std::scoped_lock<SpinLock> lock2(m_LockBundles);
		m_SegmentsToSend[0] = {};
		m_SegmentsToSend[1] = {};
		m_SegmentsWaitingForAck.clear();
		m_Bundles.clear();

		m_SegmentPool.Reset();
		m_Statistics.Reset();
		m_QueueIndex = 0;
	}

	void PacketManagerBase::QueryBegin(PacketTransceiverBase* pTransceiver, TArray<NetworkSegment*>& segmentsReturned)
	{
		TArray<NetworkSegment*> segments;
		TArray<uint32> acks;

		if (!pTransceiver->ReceiveEnd(&m_SegmentPool, segments, acks, &m_Statistics))
			return;

		segmentsReturned.Clear();
		segmentsReturned.Reserve(segments.GetSize());

		HandleAcks(acks);
		FindSegmentsToReturn(segments, segmentsReturned);

		LOG_MESSAGE("PING %fms", GetStatistics()->GetPing().AsMilliSeconds());
	}

	/*void PacketManagerBase::FindSegmentsToReturn(const TArray<NetworkSegment*>& segmentsReceived, TArray<NetworkSegment*>& segmentsReturned)
	{
		bool runUntangler = false;
		bool hasReliableMessage = false;

		TArray<NetworkSegment*> packetsToFree;
		packetsToFree.Reserve(32);

		for (NetworkSegment* pPacket : segmentsReceived)
		{
			if (!pPacket->IsReliable())																//Unreliable Packet
			{
				if (pPacket->GetType() == NetworkSegment::TYPE_NETWORK_ACK)
					packetsToFree.PushBack(pPacket);
				else
					segmentsReturned.PushBack(pPacket);
			}
			else
			{
				hasReliableMessage = true;

				if (pPacket->GetReliableUID() == m_Statistics.GetLastReceivedReliableUID() + 1)		//Reliable Packet in correct order
				{
					segmentsReturned.PushBack(pPacket);
					m_Statistics.RegisterReliableSegmentReceived();
					runUntangler = true;
				}
				else if (pPacket->GetReliableUID() > m_Statistics.GetLastReceivedReliableUID())		//Reliable Packet in incorrect order
				{
					m_ReliableSegmentsReceived.insert(pPacket);
					runUntangler = true;
				}
				else																				//Reliable Packet already received before
				{
					packetsToFree.PushBack(pPacket);
				}
			}
		}

		m_SegmentPool.FreeSegments(packetsToFree);

		if (runUntangler)
			UntangleReliableSegments(segmentsReturned);

		if (hasReliableMessage && m_SegmentsToSend[m_QueueIndex].empty())
			EnqueueSegmentUnreliable(m_SegmentPool.RequestFreeSegment()->SetType(NetworkSegment::TYPE_NETWORK_ACK));
	}*/


	/*
	* Finds packets that have been sent erlier and are now acked.
	* Notifies the listener that the packet was succesfully delivered.
	* Removes the packet and returns it to the pool.
	*/
	void PacketManagerBase::HandleAcks(const TArray<uint32>& acks)
	{
		TArray<uint32> ackedReliableUIDs;
		GetReliableUIDsFromAcks(acks, ackedReliableUIDs);

		TArray<SegmentInfo> messagesAcked;
		GetReliableSegmentInfosFromUIDs(ackedReliableUIDs, messagesAcked);

		TArray<NetworkSegment*> packetsToFree;
		packetsToFree.Reserve(messagesAcked.GetSize());

		for (SegmentInfo& messageInfo : messagesAcked)
		{
			if (messageInfo.Listener)
			{
				messageInfo.Listener->OnPacketDelivered(messageInfo.Packet);
			}
			packetsToFree.PushBack(messageInfo.Packet);
		}

		m_SegmentPool.FreeSegments(packetsToFree);
	}

	void PacketManagerBase::GetReliableUIDsFromAcks(const TArray<uint32>& acks, TArray<uint32>& ackedReliableUIDs)
	{
		ackedReliableUIDs.Reserve(128);
		std::scoped_lock<SpinLock> lock(m_LockBundles);

		Timestamp timestamp = 0;

		for (uint32 ack : acks)
		{
			auto iterator = m_Bundles.find(ack);
			if (iterator != m_Bundles.end())
			{
				Bundle& bundle = iterator->second;
				for (uint32 UID : bundle.ReliableUIDs)
					ackedReliableUIDs.PushBack(UID);

				timestamp = bundle.Timestamp;
				m_Bundles.erase(iterator);
			}
		}

		if (timestamp != 0)
		{
			RegisterRTT(EngineLoop::GetTimeSinceStart() - timestamp);
		}
	}

	void PacketManagerBase::GetReliableSegmentInfosFromUIDs(const TArray<uint32>& ackedReliableUIDs, TArray<SegmentInfo>& ackedReliableSegments)
	{
		ackedReliableSegments.Reserve(128);
		std::scoped_lock<SpinLock> lock(m_LockSegmentsToSend);

		for (uint32 UID : ackedReliableUIDs)
		{
			auto iterator = m_SegmentsWaitingForAck.find(UID);
			if (iterator != m_SegmentsWaitingForAck.end())
			{
				ackedReliableSegments.PushBack(iterator->second);
				m_SegmentsWaitingForAck.erase(iterator);
			}
		}
	}

	void PacketManagerBase::RegisterRTT(Timestamp rtt)
	{
		static const double scalar1 = 1.0f / 5.0f;
		static const double scalar2 = 1.0f - scalar1;
		m_Statistics.m_Ping = (uint64)((rtt.AsNanoSeconds() * scalar1) + (m_Statistics.GetPing().AsNanoSeconds() * scalar2));
	}
}