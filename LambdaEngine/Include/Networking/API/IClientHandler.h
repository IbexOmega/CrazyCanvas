#pragma once

#include "LambdaEngine.h"
#include "IClientRemoteHandler.h"

namespace LambdaEngine
{
	class LAMBDA_API IClientHandler : public IClientRemoteHandler
	{
	public:
		DECL_INTERFACE(IClientHandler);

		virtual void OnServerFull(IClient* pClient) = 0;
	};
}