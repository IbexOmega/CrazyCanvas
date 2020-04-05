#pragma once

#include "Defines.h"

namespace LambdaEngine
{
	class ClientUDP;
	class NetworkPacket;

	class LAMBDA_API IClientUDPHandler
	{
	public:
		DECL_INTERFACE(IClientUDPHandler);

		/*
		* Called when a client receives a packet
		*
		* client  - The client
		* packet  - The packet received
		*/
		virtual void OnClientPacketReceivedUDP(ClientUDP* client, NetworkPacket* packet) = 0;
	};
}
