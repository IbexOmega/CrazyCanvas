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

	bool PacketManagerTCP::FindSegmentsToReturn(const TArray<NetworkSegment*>& segmentsReceived, TArray<NetworkSegment*>& segmentsReturned)
	{
		bool hasReliableSegment = false;
		TArray<NetworkSegment*> packetsToFree;
		packetsToFree.Reserve(32);

		for (NetworkSegment* pSegment : segmentsReceived)
		{
			if (!pSegment->IsReliable())
			{
				if (pSegment->GetType() == NetworkSegment::TYPE_NETWORK_ACK)
					packetsToFree.PushBack(pSegment);
				else
					segmentsReturned.PushBack(pSegment);
			}
			else
			{
				segmentsReturned.PushBack(pSegment);
				hasReliableSegment = true;
			}
		}

		m_SegmentPool.FreeSegments(packetsToFree, "PacketManagerTCP::FindSegmentsToReturn");


#ifdef LAMBDA_CONFIG_DEBUG
		if (hasReliableSegment && m_SegmentsToSend[m_QueueIndex].empty())
			EnqueueSegmentUnreliable(m_SegmentPool.RequestFreeSegment("PacketManagerTCP_NETWORK_ACK")->SetType(NetworkSegment::TYPE_NETWORK_ACK));
#else
		if (hasReliableSegment && m_SegmentsToSend[m_QueueIndex].empty())
			EnqueueSegmentUnreliable(m_SegmentPool.RequestFreeSegment()->SetType(NetworkSegment::TYPE_NETWORK_ACK));
#endif
		return false;
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