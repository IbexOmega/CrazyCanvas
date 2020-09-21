#pragma once

#include "LambdaEngine.h"

#include "Networking/API/BinaryDecoder.h"

namespace LambdaEngine
{
	class NetworkSegment;

	class LAMBDA_API INetworkDiscoveryClient
	{
	public:
		DECL_INTERFACE(INetworkDiscoveryClient);

		virtual void OnServerFound(BinaryDecoder decoder, uint16 portOfGameServer) = 0;
	};
}