#pragma once

#include "Defines.h"
#include "EProtocol.h"

namespace LambdaEngine
{
	class ISocket;

	class LAMBDA_API ISocketFactory
	{
	public:
		DECL_INTERFACE(ISocketFactory);

		virtual bool Init() = 0;
		virtual void Release() = 0;
		virtual ISocket* CreateSocket(EProtocol protocol) = 0;
	};
}
