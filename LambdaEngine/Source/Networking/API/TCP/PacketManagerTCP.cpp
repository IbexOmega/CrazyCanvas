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

	void PacketManagerTCP::FindSegmentsToReturn(const TArray<NetworkSegment*>& segmentsReceived, TArray<NetworkSegment*>& segmentsReturned)
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

		m_SegmentPool.FreeSegments(packetsToFree);

		if (hasReliableSegment && m_SegmentsToSend[m_QueueIndex].empty())
			EnqueueSegmentUnreliable(m_SegmentPool.RequestFreeSegment()->SetType(NetworkSegment::TYPE_NETWORK_ACK));
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