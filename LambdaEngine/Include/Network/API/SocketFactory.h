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

		static ISocketTCP* CreateSocketTCP() { return nullptr; };
		static ISocketUDP* CreateSocketUDP() { return nullptr; };

	private:
		static bool Init() { return false; };
		static void Release() {};
	};
}
