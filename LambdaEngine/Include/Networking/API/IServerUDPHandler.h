#pragma once

#include "LambdaEngine.h"

namespace LambdaEngine
{
	class IClientUDP;
	class IClientUDPHandler;

	class LAMBDA_API IServerUDPHandler
	{
	public:
		DECL_INTERFACE(IServerUDPHandler);

		virtual void OnClientConnected(IClientUDP* pClient) = 0;
		virtual IClientUDPHandler* CreateClientUDPHandler() = 0;
	};
}