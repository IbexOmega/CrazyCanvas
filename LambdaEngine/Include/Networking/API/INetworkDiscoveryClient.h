#pragma once

#include "LambdaEngine.h"

namespace LambdaEngine
{
	class NetworkSegment;

	class LAMBDA_API INetworkDiscoveryClient
	{
	public:
		DECL_INTERFACE(INetworkDiscoveryClient);

		virtual void OnServerFound(NetworkSegment* pPacket) = 0;
	};
}