#include "Networking/API/TCP/PacketManagerTCP.h"

#include "Networking/API/NetworkSegment.h"
#include "Networking/API/IPacketListener.h"
#include "Networking/API/PacketTransceiverBase.h"


#include "Engine/EngineLoop.h"

namespace LambdaEngine
{
	PacketManagerTCP::PacketManagerTCP(const PacketManagerDesc& desc) :
		PacketManagerBase(desc)
	{

	}

	PacketManagerTCP::~PacketManagerTCP()
	{

	}

	bool PacketManagerTCP::FindSegmentsToReturn(const TArray<NetworkSegment*>& segmentsReceived, TArray<NetworkSegment*>& segmentsReturned, bool& hasDiscardedResends)
	{
		bool hasReliableSegment = false;
		hasDiscardedResends = false;
		TArray<NetworkSegment*> packetsToFree;
		packetsToFree.Reserve(32);

		for (NetworkSegment* pSegment : segmentsReceived)
		{
			if (!pSegment->IsReliable())
			{
				if (pSegment->GetType() == NetworkSegment::TYPE_NETWORK_ACK)
				{
					packetsToFree.PushBack(pSegment);
				}
				else
				{
					segmentsReturned.PushBack(pSegment);
					m_Statistics.RegisterUniqueSegmentReceived(pSegment->GetType());
				}
			}
			else
			{
				segmentsReturned.PushBack(pSegment);
				m_Statistics.RegisterUniqueSegmentReceived(pSegment->GetType());
				hasReliableSegment = true;
			}
		}

#ifdef LAMBDA_CONFIG_DEBUG
		m_SegmentPool.FreeSegments(packetsToFree, "PacketManagerTCP::FindSegmentsToReturn");
#else
		m_SegmentPool.FreeSegments(packetsToFree);
#endif


		if (hasReliableSegment && m_SegmentsToSend[m_QueueIndex].empty())
		{
#ifdef LAMBDA_CONFIG_DEBUG
			NetworkSegment* pSegment = m_SegmentPool.RequestFreeSegment("PacketManagerTCP_NETWORK_ACK");
#else
			NetworkSegment* pSegment = m_SegmentPool.RequestFreeSegment();
#endif
			if (pSegment)
				EnqueueSegmentUnreliable(pSegment->SetType(NetworkSegment::TYPE_NETWORK_ACK));
			else
				return false;
		}
		return true;
	}


	void PacketManagerTCP::Tick(Timestamp delta)
	{
		PacketManagerBase::Tick(delta);
	}

	void PacketManagerTCP::Reset()
	{
		PacketManagerBase::Reset();
	}
}