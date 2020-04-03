#pragma once

#include "Network/API/SocketFactory.h"

namespace LambdaEngine
{
	class LAMBDA_API Win32SocketFactory : public SocketFactory
	{
		friend class EngineLoop;
        
	public:
		static ISocketTCP* CreateSocketTCP();
		static ISocketUDP* CreateSocketUDP();
		static const std::string& GetLocalAddress();

    private:
        static bool Init();
        static void Release();
	};

    typedef Win32SocketFactory PlatformSocketFactory;
}