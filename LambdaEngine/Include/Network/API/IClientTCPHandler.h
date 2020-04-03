#pragma once

#include "Defines.h"

namespace LambdaEngine
{
	class ClientTCP;
	class NetworkPacket;

	class LAMBDA_API IClientTCPHandler
	{
	public:
		DECL_INTERFACE(IClientTCPHandler);

		virtual void OnClientConnected(ClientTCP* client) = 0;
		virtual void OnClientDisconnected(ClientTCP* client) = 0;
		virtual void OnClientFailedConnecting(ClientTCP* client) = 0;
		virtual void OnClientPacketReceived(ClientTCP* client, NetworkPacket* packet) = 0;
	};
}
