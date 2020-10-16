#pragma once

#include "LambdaEngine.h"

#include "Networking/API/BinaryDecoder.h"
#include "Networking/API/IPEndPoint.h"

namespace LambdaEngine
{

	class LAMBDA_API INetworkDiscoveryClient
	{
	public:
		DECL_INTERFACE(INetworkDiscoveryClient);

		virtual void OnServerFound(BinaryDecoder& decoder, const IPEndPoint& endPoint, uint64 serverUID) = 0;
	};
}
