#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Network/Win32/Win32NetworkUtils.h"
#include "Network/Win32/Win32SocketTCP.h"
#include "Network/Win32/Win32SocketUDP.h"

#include "Log/Log.h"
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

namespace LambdaEngine
{
	bool Win32NetworkUtils::Init()
	{
		WSADATA wsa;

		if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		{
			LOG_ERROR("Failed to initialize Winsock2, Error Code : %d", WSAGetLastError());
			return false;
		}
		LOG_INFO("[Winsock2]: Initialised");
		return NetworkUtils::Init();
	}

	void Win32NetworkUtils::Release()
	{
		WSACleanup();
		LOG_INFO("[Winsock2]: Released");
		NetworkUtils::Release();
	}

	ISocketTCP* Win32NetworkUtils::CreateSocketTCP()
	{
		return DBG_NEW Win32SocketTCP();
	}

	ISocketUDP* Win32NetworkUtils::CreateSocketUDP()
	{
		return DBG_NEW Win32SocketUDP();
	}

	std::string Win32NetworkUtils::GetLocalAddress()
	{
		return NetworkUtils::GetLocalAddress();
	}
}
#endif
