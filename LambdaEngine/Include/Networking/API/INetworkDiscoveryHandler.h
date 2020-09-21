#pragma once

#include "LambdaEngine.h"

namespace LambdaEngine
{
	class NetworkSegment;

	class LAMBDA_API INetworkDiscoveryHandler
	{
	public:
		DECL_INTERFACE(INetworkDiscoveryHandler);

		virtual void OnNetworkDiscoveryPreTransmit(NetworkSegment* pPacket) = 0;
	};
}