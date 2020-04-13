#pragma once

#include "Network/API/NetworkUtils.h"

namespace LambdaEngine
{
	class LAMBDA_API MacNetworkUtils : public NetworkUtils
	{
        friend class EngineLoop;
        
	public:
		/*
		* Creates a SocketTCP.
		*
		* return - a SocketTCP.
		*/
		static ISocketTCP* CreateSocketTCP();

		/*
		* Creates a SocketUDP.
		*
		* return - a SocketUDP.
		*/
		static ISocketUDP* CreateSocketUDP();

    private:
		static IPAddress* CreateIPAddress(const std::string& address, uint64 hash);

        static bool Init();
        static void Release();
	};

    typedef MacNetworkUtils PlatformNetworkUtils;
}