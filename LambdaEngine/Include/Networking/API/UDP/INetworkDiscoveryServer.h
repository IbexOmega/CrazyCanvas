#pragma once

#include "LambdaEngine.h"

#include "Networking/API/BinaryEncoder.h"

namespace LambdaEngine
{
	class LAMBDA_API INetworkDiscoveryServer
	{
	public:
		DECL_INTERFACE(INetworkDiscoveryServer);

		virtual void OnNetworkDiscoveryPreTransmit(const BinaryEncoder& encoder) = 0;
	};
}