#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Networking/Win32/Win32NetworkUtils.h"
#include "Networking/Win32/Win32SocketTCP.h"
#include "Networking/Win32/Win32SocketUDP.h"
#include "Networking/Win32/Win32IPAddress.h"

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
		NetworkUtils::Release();
	}

	void Win32NetworkUtils::PostRelease()
	{
		NetworkUtils::PostRelease();
		WSACleanup();
		LOG_INFO("[Winsock2]: Released");
	}

	ISocketTCP* Win32NetworkUtils::CreateSocketTCP()
	{
		return DBG_NEW Win32SocketTCP();
	}

	ISocketUDP* Win32NetworkUtils::CreateSocketUDP()
	{
		return DBG_NEW Win32SocketUDP();
	}

	IPAddress* Win32NetworkUtils::CreateIPAddress(const std::string& address, uint64 hash)
	{
		return DBG_NEW Win32IPAddress(address, hash);
	}
}
#endif
