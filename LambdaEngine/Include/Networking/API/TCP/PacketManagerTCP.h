#pragma once

#include "Networking/API/PacketManagerBase.h"

namespace LambdaEngine
{
	class LAMBDA_API PacketManagerTCP : public PacketManagerBase
	{
	public:
		PacketManagerTCP(const PacketManagerDesc& desc);
		~PacketManagerTCP();

		virtual void Tick(Timestamp delta) override;

		virtual void Reset() override;

	protected:
		virtual void FindSegmentsToReturn(const TArray<NetworkSegment*>& segmentsReceived, TArray<NetworkSegment*>& segmentsReturned) override;
	};
}
