#pragma once

#include "LambdaEngine.h"
#include "IClientUDPRemoteHandler.h"

namespace LambdaEngine
{
	class LAMBDA_API IClientUDPHandler : public IClientUDPRemoteHandler
	{
	public:
		DECL_INTERFACE(IClientUDPHandler);

		virtual void OnServerFullUDP(IClientUDP* pClient) = 0;
	};
}