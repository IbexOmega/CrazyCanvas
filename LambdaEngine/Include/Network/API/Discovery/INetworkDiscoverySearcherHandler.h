#pragma once
#include "Defines.h"
#include "Types.h"

#include "Containers/String.h"

#include "Network/API/NetworkPacket.h"

namespace LambdaEngine
{
	class LAMBDA_API INetworkDiscoverySearcherHandler
	{
	public:
		DECL_INTERFACE(INetworkDiscoverySearcherHandler);

		/*
		* Called when a NetworkDiscoveryHost responds to a search request
		* Use this function to gather the required inet address and port, as well as extra data in the packet.
		*
		* address - The address sent by the host
		* port    - The port sent by the host
		* packet  - A packet with extra data sent by the host
		*/
		virtual void OnHostFound(const std::string& address, uint16 port, NetworkPacket* packet) = 0;
	};
}
