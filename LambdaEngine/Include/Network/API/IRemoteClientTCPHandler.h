#pragma once

#include "Defines.h"

namespace LambdaEngine
{
	class ClientTCP;

	class LAMBDA_API IRemoteClientTCPHandler
	{
	public:
		DECL_INTERFACE(IRemoteClientTCPHandler);

		/*
		* Called when a client for some reason is disconnected.
		*
		* client  - The client
		*/
		virtual void OnClientDisconnected(ClientTCP* client) = 0;
	};
}
