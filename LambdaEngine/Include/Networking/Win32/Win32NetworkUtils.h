#pragma once

#include "Networking/API/NetworkUtils.h"

namespace LambdaEngine
{
	class LAMBDA_API Win32NetworkUtils : public NetworkUtils
	{
		friend class EngineLoop;
		friend class IPAddress;
        
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
        static void PreRelease();
		static void PostRelease();
	};

    typedef Win32NetworkUtils PlatformNetworkUtils;
}