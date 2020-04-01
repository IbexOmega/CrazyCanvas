#pragma once

#include "Defines.h"

namespace LambdaEngine
{
	class ISocketTCP;
	class ISocketUDP;

	class LAMBDA_API SocketFactory
	{
	public:
		DECL_INTERFACE(SocketFactory);

		static ISocketTCP* CreateSocketTCP() {};
		static ISocketUDP* CreateSocketUDP() {};

	private:
		static bool Init() {};
		static void Release() {};
	};
}
