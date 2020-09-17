#pragma once

#include "LambdaEngine.h"

namespace LambdaEngine
{
	class IClientRemoteHandler;

	class LAMBDA_API IServerHandler
	{
	public:
		DECL_INTERFACE(IServerHandler);

		virtual IClientRemoteHandler* CreateClientHandler() = 0;
	};
}