#pragma once

#include "LambdaEngine.h"

namespace LambdaEngine
{
	class IClient;
	class IClientRemoteHandler;

	class LAMBDA_API IServerHandler
	{
	public:
		DECL_INTERFACE(IServerHandler);

		virtual void OnClientConnected(IClient* pClient) = 0;
		virtual IClientRemoteHandler* CreateClientHandler() = 0;
	};
}