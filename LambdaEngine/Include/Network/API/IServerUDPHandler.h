#pragma once

#include "Defines.h"
#include "IClientTCPHandler.h"

namespace LambdaEngine
{
	class ClientTCP;

	class LAMBDA_API IServerUDPHandler
	{
	public:
		DECL_INTERFACE(IServerUDPHandler);

		/*
		* Called to create a client handler for a ClientTCP object
		*
		* return  - a new handler
		*/
		virtual IClientTCPHandler* CreateClientHandler() = 0;

		/*
		* Called when a client is accepted by the server.
		*
		* client  - The accepted client
		*
		* return  - true to keep the client, otherwise false.
		*/
		virtual bool OnClientAccepted(ClientTCP* client) = 0;

		/*
		* Called after OnClientAccepted if keept.
		*
		* client  - The client
		*/
		virtual void OnClientConnected(ClientTCP* client) = 0;

		/*
		* Called when a client for some reason is disconnected.
		*
		* client  - The client
		*/
		virtual void OnClientDisconnected(ClientTCP* client) = 0;
	};
}
