#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Network/Win32/Win32SocketFactory.h"
#include "Network/Win32/Win32Socket.h"
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

		LOG_MESSAGE("Winsock2 Initialised");
		return true;
	}

	void Win32SocketFactory::Release()
	{
		WSACleanup();
	}

	ISocket* Win32SocketFactory::CreateSocket(EProtocol protocol)
	{
		Win32Socket* socket = new Win32Socket();
		socket->Init(protocol);
		return socket;
	}
}
#endif
