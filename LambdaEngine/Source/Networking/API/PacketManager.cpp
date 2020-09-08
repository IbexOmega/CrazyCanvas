#include "Networking/API/PacketManager.h"

#include "Networking/API/NetworkSegment.h"
#include "Networking/API/IPacketListener.h"
#include "Networking/API/PacketTransceiver.h"

#include "Engine/EngineLoop.h"

namespace LambdaEngine
{
	PacketManager::PacketManager(const PacketManagerDesc& desc) :
		m_SegmentPool(desc.PoolSize),
		m_QueueIndex(0),
		m_MaxRetries(desc.MaxRetries),
		m_ResendRTTMultiplier(desc.ResendRTTMultiplier)
	{

	}

	PacketManager::~PacketManager()
	{

	}

	uint32 PacketManager::EnqueueSegmentReliable(NetworkSegment* pSegment, IPacketListener* pListener)
	{
		std::scoped_lock<SpinLock> lock(m_LockSegmentsToSend);
		uint32 UID = EnqueueSegment(pSegment, m_Statistics.RegisterReliableSegmentSent());
		m_SegmentsWaitingForAck.insert({ UID, SegmentInfo{ pSegment, pListener, EngineLoop::GetTimeSinceStart()} });
		return UID;
	}

	uint32 PacketManager::EnqueueSegmentUnreliable(NetworkSegment* pSegment)
	{
		std::scoped_lock<SpinLock> lock(m_LockSegmentsToSend);
		return EnqueueSegment(pSegment, 0);
	}

	uint32 PacketManager::EnqueueSegment(NetworkSegment* pSegment, uint32 reliableUID)
	{
		pSegment->GetHeader().UID = m_Statistics.RegisterSegmentSent();
		pSegment->GetHeader().ReliableUID = reliableUID;
		m_SegmentsToSend[m_QueueIndex].push(pSegment);
		return pSegment->GetHeader().UID;
	}

	void PacketManager::Flush(PacketTransceiver* pTransceiver)
	{
		int32 indexToUse = m_QueueIndex;
		m_QueueIndex = (m_QueueIndex + 1) % 2;
		std::queue<NetworkSegment*>& packets = m_SegmentsToSend[indexToUse];

		Timestamp timestamp = EngineLoop::GetTimeSinceStart();

		while (!packets.empty())
		{
			Bundle bundle;
			uint32 bundleUID = pTransceiver->Transmit(&m_SegmentPool, packets, bundle.ReliableUIDs, m_IPEndPoint, &m_Statistics);

			if (!bundle.ReliableUIDs.empty())
			{
				bundle.Timestamp = timestamp;

				std::scoped_lock<SpinLock> lock(m_LockBundles);
				m_Bundles.insert({ bundleUID, bundle });
			}
		}
	}

	void PacketManager::QueryBegin(PacketTransceiver* pTransceiver, TArray<NetworkSegment*>& segmentsReturned)
	{
		TArray<NetworkSegment*> packets;
		IPEndPoint ipEndPoint;
		TArray<uint32> acks;

		if (!pTransceiver->ReceiveEnd(&m_SegmentPool, packets, acks, &m_Statistics))
			return;

		segmentsReturned.Clear();
		segmentsReturned.Reserve(packets.GetSize());

		HandleAcks(acks);
		FindSegmentsToReturn(packets, segmentsReturned);

		LOG_MESSAGE("PING %fms", GetStatistics()->GetPing().AsMilliSeconds());
	}

	void PacketManager::QueryEnd(TArray<NetworkSegment*>& segmentsReceived)
	{
		m_SegmentPool.FreeSegments(segmentsReceived);
	}

	void PacketManager::Tick(Timestamp delta)
	{
		static const Timestamp delay = Timestamp::Seconds(1);

		m_Timer += delta;
		if (m_Timer >= delay)
		{
			m_Timer -= delay;
			DeleteOldBundles();
		}

		ResendOrDeleteSegments();
	}

	SegmentPool* PacketManager::GetSegmentPool()
	{
		return &m_SegmentPool;
	}

	const NetworkStatistics* PacketManager::GetStatistics() const
	{
		return &m_Statistics;
	}

	const IPEndPoint& PacketManager::GetEndPoint() const
	{
		return m_IPEndPoint;
	}

	void PacketManager::SetEndPoint(const IPEndPoint& ipEndPoint)
	{
		m_IPEndPoint = ipEndPoint;
	}

	void PacketManager::Reset()
	{
		std::scoped_lock<SpinLock> lock1(m_LockSegmentsToSend);
		std::scoped_lock<SpinLock> lock2(m_LockBundles);
		m_SegmentsToSend[0] = {};
		m_SegmentsToSend[1] = {};
		m_SegmentsWaitingForAck.clear();
		m_ReliableSegmentsReceived.clear();
		m_Bundles.clear();

		m_SegmentPool.Reset();
		m_Statistics.Reset();
		m_QueueIndex = 0;
	}

	void PacketManager::FindSegmentsToReturn(const TArray<NetworkSegment*>& segmentsReceived, TArray<NetworkSegment*>& segmentsReturned)
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
	}

	void PacketManager::UntangleReliableSegments(TArray<NetworkSegment*>& segmentsReturned)
	{
		TArray<NetworkSegment*> packetsToErase;

		for (NetworkSegment* pPacket : m_ReliableSegmentsReceived)
		{
			if (pPacket->GetReliableUID() == m_Statistics.GetLastReceivedReliableUID() + 1)
			{
				segmentsReturned.PushBack(pPacket);
				packetsToErase.PushBack(pPacket);
				m_Statistics.RegisterReliableSegmentReceived();
			}
			else
			{
				break;
			}
		}

		for (NetworkSegment* pPacket : packetsToErase)
		{
			m_ReliableSegmentsReceived.erase(pPacket);
		}
	}

	/*
	* Finds packets that have been sent erlier and are now acked.
	* Notifies the listener that the packet was succesfully delivered.
	* Removes the packet and returns it to the pool.
	*/
	void PacketManager::HandleAcks(const TArray<uint32>& acks)
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

	void PacketManager::GetReliableUIDsFromAcks(const TArray<uint32>& acks, TArray<uint32>& ackedReliableUIDs)
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

	void PacketManager::GetReliableSegmentInfosFromUIDs(const TArray<uint32>& ackedReliableUIDs, TArray<SegmentInfo>& ackedReliableSegments)
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

	void PacketManager::RegisterRTT(Timestamp rtt)
	{
		static const double scalar1 = 1.0f / 5.0f;
		static const double scalar2 = 1.0f - scalar1;
		m_Statistics.m_Ping = (uint64)((rtt.AsNanoSeconds() * scalar1) + (m_Statistics.GetPing().AsNanoSeconds() * scalar2));
	}

	void PacketManager::DeleteOldBundles()
	{
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

	void PacketManager::ResendOrDeleteSegments()
	{
		static Timestamp minTime = Timestamp::MilliSeconds(5);
		uint64 pingNanos = m_Statistics.GetPing().AsNanoSeconds();
		Timestamp maxAllowedTime = Timestamp(pingNanos * m_ResendRTTMultiplier);
		if (maxAllowedTime < minTime)
			maxAllowedTime = minTime;

		Timestamp currentTime = EngineLoop::GetTimeSinceStart();

		TArray<std::pair<const uint32, SegmentInfo>> messagesToDelete;

		{
			std::scoped_lock<SpinLock> lock(m_LockSegmentsToSend);

			for (auto& pair : m_SegmentsWaitingForAck)
			{
				SegmentInfo& messageInfo = pair.second;
				if (currentTime - messageInfo.LastSent > maxAllowedTime)
				{
					messageInfo.Retries++;

					if (messageInfo.Retries < m_MaxRetries)
					{
						m_SegmentsToSend[m_QueueIndex].push(messageInfo.Packet);
						messageInfo.LastSent = currentTime;

						if (messageInfo.Listener)
							messageInfo.Listener->OnPacketResent(messageInfo.Packet, messageInfo.Retries);
					}
					else
					{
						messagesToDelete.PushBack(pair);
					}
				}
			}

			for (auto& pair : messagesToDelete)
				m_SegmentsWaitingForAck.erase(pair.first);
		}
		
		TArray<NetworkSegment*> packetsToFree;
		packetsToFree.Reserve(messagesToDelete.GetSize());

		for (auto& pair : messagesToDelete)
		{
			SegmentInfo& messageInfo = pair.second;
			packetsToFree.PushBack(messageInfo.Packet);
			if (messageInfo.Listener)
				messageInfo.Listener->OnPacketMaxTriesReached(messageInfo.Packet, messageInfo.Retries);
		}

		m_SegmentPool.FreeSegments(packetsToFree);
	}
}