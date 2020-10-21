#include "Networking/API/UDP/PacketManagerUDP.h"

#include "Networking/API/NetworkSegment.h"
#include "Networking/API/IPacketListener.h"
#include "Networking/API/PacketTransceiverBase.h"

#include "Engine/EngineLoop.h"

namespace LambdaEngine
{
	PacketManagerUDP::PacketManagerUDP(const PacketManagerDesc& desc) : 
		PacketManagerBase(desc),
		m_MaxRetries(desc.MaxRetries),
		m_ResendRTTMultiplier(desc.ResendRTTMultiplier)
	{

	}

	PacketManagerUDP::~PacketManagerUDP()
	{

	}

	void PacketManagerUDP::Tick(Timestamp delta)
	{
		PacketManagerBase::Tick(delta);
		ResendOrDeleteSegments();
	}

	void PacketManagerUDP::Reset()
	{
		PacketManagerBase::Reset();
		m_ReliableSegmentsReceived.clear();
	}

	bool PacketManagerUDP::FindSegmentsToReturn(const TArray<NetworkSegment*>& segmentsReceived, TArray<NetworkSegment*>& segmentsReturned)
	{
		bool runUntangler = false;
		bool hasReliableSegment = false;
		bool hasDuplicateSegment = false;

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
				hasReliableSegment = true;

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
					hasDuplicateSegment = true;
				}
			}
		}
		
#ifdef LAMBDA_CONFIG_DEBUG
		m_SegmentPool.FreeSegments(packetsToFree, "PacketManagerTCP::FindSegmentsToReturn");
#else
		m_SegmentPool.FreeSegments(packetsToFree);
#endif

		if (runUntangler)
			UntangleReliableSegments(segmentsReturned);

#ifdef LAMBDA_CONFIG_DEBUG
		if (hasReliableSegment && m_SegmentsToSend[m_QueueIndex].empty())
			EnqueueSegmentUnreliable(m_SegmentPool.RequestFreeSegment("PacketManagerUDP_NETWORK_ACK")->SetType(NetworkSegment::TYPE_NETWORK_ACK));
#else
		if (hasReliableSegment && m_SegmentsToSend[m_QueueIndex].empty())
			EnqueueSegmentUnreliable(m_SegmentPool.RequestFreeSegment()->SetType(NetworkSegment::TYPE_NETWORK_ACK));
#endif
		return hasDuplicateSegment;
	}

	void PacketManagerUDP::UntangleReliableSegments(TArray<NetworkSegment*>& segmentsReturned)
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
		}

		for (NetworkSegment* pPacket : packetsToErase)
		{
			m_ReliableSegmentsReceived.erase(pPacket);
		}
	}

	void PacketManagerUDP::ResendOrDeleteSegments()
	{
		static Timestamp minTime = Timestamp::MilliSeconds(5);
		float64 pingNanos = (float32)m_Statistics.GetPing().AsNanoSeconds();
		Timestamp maxAllowedTime = Timestamp((uint64)(pingNanos * (float64)m_ResendRTTMultiplier));
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
						InsertSegment(messageInfo.Segment);
						messageInfo.LastSent = currentTime;
						if (messageInfo.Listener)
							messageInfo.Listener->OnPacketResent(messageInfo.Segment, messageInfo.Retries);
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
		
		for (auto& pair : messagesToDelete)
		{
			SegmentInfo& messageInfo = pair.second;

			if (messageInfo.Listener)
				messageInfo.Listener->OnPacketMaxTriesReached(messageInfo.Segment, messageInfo.Retries);

#ifdef LAMBDA_CONFIG_DEBUG
			m_SegmentPool.FreeSegment(messageInfo.Segment, "PacketManagerUDP::ResendOrDeleteSegments");
#else
			m_SegmentPool.FreeSegment(messageInfo.Segment);
#endif	
		}
	}
}