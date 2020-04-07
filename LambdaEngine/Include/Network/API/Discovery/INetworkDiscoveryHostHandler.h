#pragma once

#include "Defines.h"
#include "Types.h"
#include <string>

#include "Network/API/NetworkPacket.h"

namespace LambdaEngine
{
	class LAMBDA_API INetworkDiscoveryHostHandler
	{
	public:
		DECL_INTERFACE(INetworkDiscoveryHostHandler);

		/*
		* Called when a searcher asks for details.
		* Use this function to write extra data to be sent to the searcher.
		* 
		* packet - The packet to be sent back to the searcher
		*/
		virtual void OnSearcherRequest(NetworkPacket* packet) = 0;
	};
}
