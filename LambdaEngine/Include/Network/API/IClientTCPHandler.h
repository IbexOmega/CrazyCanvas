#pragma once

#include "Defines.h"

namespace LambdaEngine
{
	class ClientTCP2;
	class NetworkPacket;

	class LAMBDA_API IClientTCPHandler
	{
	public:
		DECL_INTERFACE(IClientTCPHandler);

		/*
		* Called when a successful connection has been made
		*
		* client  - The client
		*/
		virtual void OnClientConnected(ClientTCP2* client) = 0;

		/*
		* Called when a client for some reason is disconnected.
		*
		* client  - The client
		*/
		virtual void OnClientDisconnected(ClientTCP2* client) = 0;

		/*
		* Called when a client fails to connect
		*
		* client  - The client
		*/
		virtual void OnClientFailedConnecting(ClientTCP2* client) = 0;

		/*
		* Called when a client receives a packet
		*
		* client  - The client
		* packet  - The packet received
		*/
		virtual void OnClientPacketReceived(ClientTCP2* client, NetworkPacket* packet) = 0;
	};
}
