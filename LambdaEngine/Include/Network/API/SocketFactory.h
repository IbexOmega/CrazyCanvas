#pragma once

#include "Defines.h"
#include <string>

namespace LambdaEngine
{
	class ISocketTCP;
	class ISocketUDP;

	class LAMBDA_API SocketFactory
	{
	public:
		DECL_INTERFACE(SocketFactory);

	public:
		static ISocketTCP* CreateSocketTCP() { return nullptr; };
		static ISocketUDP* CreateSocketUDP() { return nullptr; };
		static const std::string& GetLocalAddress() { return ""; };

	private:
		static bool Init() { return false; };
		static void Release() {};
	};
}
