#pragma once

#include "Defines.h"

namespace LambdaEngine
{
	class ISocketTCP;
	class ISocketUDP;

	class LAMBDA_API ISocketFactory
	{
	public:
		DECL_INTERFACE(ISocketFactory);

		virtual bool Init() = 0;
		virtual void Release() = 0;
		virtual ISocketTCP* CreateSocketTCP() = 0;
		virtual ISocketUDP* CreateSocketUDP() = 0;
	};
}
