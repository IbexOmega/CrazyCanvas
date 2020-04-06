#pragma once

#include "Defines.h"

namespace LambdaEngine
{
	class IClientUDP;
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
		virtual void OnClientPacketReceivedUDP(IClientUDP* client, NetworkPacket* packet) = 0;

		/*
		* Called when a client receives an error
		*
		* client  - The client
		*/
		virtual void OnClientErrorUDP(IClientUDP* client) = 0;

		/*
		* Called when a client stops from either Release or an error.
		*
		* client  - The client
		*/
		virtual void OnClientStoppedUDP(IClientUDP* client) = 0;
	};
}
