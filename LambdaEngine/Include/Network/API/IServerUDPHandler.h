#pragma once

#include "Defines.h"
#include "NetworkPacket.h"

namespace LambdaEngine
{
	class LAMBDA_API IServerUDPHandler
	{
	public:
		DECL_INTERFACE(IServerUDPHandler);

		/*
		* Called when the server receives a new packet
		*
		* address - The senders inet address
		* port	  - The senders port
		*/
		virtual void OnPacketReceivedUDP(NetworkPacket* packet, const std::string& address, uint16 port) = 0;
	};
}
