#pragma once

#include "Defines.h"

namespace LambdaEngine
{
	class ClientTCP;

	class LAMBDA_API IServerTCPHandler
	{
	public:
		DECL_INTERFACE(IServerTCPHandler);

		/*
		* Return true to keep the Client otherwise false
		*/
		virtual bool OnClientAccepted(ClientTCP* client) = 0;

		virtual void OnClientConnected(ClientTCP* client) = 0;
		virtual void OnClientDisconnected(ClientTCP* client) = 0;
	};
}
