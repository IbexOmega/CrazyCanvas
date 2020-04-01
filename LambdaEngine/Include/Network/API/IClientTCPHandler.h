#pragma once

#include "Defines.h"

namespace LambdaEngine
{
	class ClientTCP;

	class LAMBDA_API IClientTCPHandler
	{
	public:
		DECL_INTERFACE(IClientTCPHandler);

		virtual void OnClientConnected(ClientTCP* client) = 0;
		virtual void OnClientDisconnected(ClientTCP* client) = 0;
		virtual void OnClientFailedConnecting(ClientTCP* client) = 0;
	};
}
