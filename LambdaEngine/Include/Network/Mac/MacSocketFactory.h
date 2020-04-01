#pragma once

#include "Network/API/SocketFactory.h"

namespace LambdaEngine
{
	class LAMBDA_API MacSocketFactory : public SocketFactory
	{
        friend class EngineLoop;
        
	public:
		static ISocketTCP* CreateSocketTCP();
		static ISocketUDP* CreateSocketUDP();

    private:
        static bool Init();
        static void Release();
	};

    typedef MacSocketFactory PlatformSocketFactory;
}