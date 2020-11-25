#pragma once

#include "Networking/API/PacketManagerBase.h"
#include "Networking/API/NetworkSegment.h"

namespace LambdaEngine
{
	class LAMBDA_API PacketManagerUDP : public PacketManagerBase
	{
	public:
		PacketManagerUDP(const PacketManagerDesc& desc);
		~PacketManagerUDP();

		virtual void Tick(Timestamp delta) override;

		virtual void Reset() override;

	protected:
		virtual bool FindSegmentsToReturn(const TArray<NetworkSegment*>& segmentsReceived, TArray<NetworkSegment*>& segmentsReturned, bool& hasDiscardedResends) override;

	private:
		void UntangleReliableSegments(TArray<NetworkSegment*>& segmentsReturned);
		void ResendOrDeleteSegments();

	private:
		std::set<NetworkSegment*, NetworkSegmentReliableUIDOrder> m_ReliableSegmentsReceived;
		float32 m_ResendRTTMultiplier;
		int32 m_MaxRetries;
	};
}
