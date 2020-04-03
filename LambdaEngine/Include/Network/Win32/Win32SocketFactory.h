#pragma once

#include "Network/API/SocketFactory.h"

namespace LambdaEngine
{
	class LAMBDA_API Win32SocketFactory : public SocketFactory
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

		/*
		* Finds the local network address. Usally 192.168.0.X
		*
		* return - The inet address
		*/
		static const std::string& GetLocalAddress();

    private:
        static bool Init();
        static void Release();
	};

    typedef Win32SocketFactory PlatformSocketFactory;
}