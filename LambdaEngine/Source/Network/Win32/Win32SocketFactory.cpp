#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Network/Win32/Win32SocketFactory.h"
#include "Network/Win32/Win32SocketTCP.h"
#include "Network/Win32/Win32SocketUDP.h"
#include "Log/Log.h"
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

namespace LambdaEngine
{
	bool Win32SocketFactory::Init()
	{
		WSADATA wsa;

		if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		{
			LOG_ERROR("Failed to initialize Winsock2, Error Code : %d", WSAGetLastError());
			return false;
		}
		LOG_INFO("[Winsock2]: Initialised");
		return SocketFactory::Init();
	}

	void Win32SocketFactory::Release()
	{
		WSACleanup();
		LOG_INFO("[Winsock2]: Released");
		SocketFactory::Release();
	}

	ISocketTCP* Win32SocketFactory::CreateSocketTCP()
	{
		return DBG_NEW Win32SocketTCP();
	}

	ISocketUDP* Win32SocketFactory::CreateSocketUDP()
	{
		return DBG_NEW Win32SocketUDP();
	}

    const std::string& Win32SocketFactory::GetLocalAddress()
    {
        ISocketUDP* socketUDP = CreateSocketUDP();
		if (socketUDP)
		{
			if (socketUDP->Connect(ADDRESS_LOOPBACK, 9))
			{
				return socketUDP->GetAddress();
			}
		}
		return ADDRESS_LOOPBACK;
    }
}
#endif
