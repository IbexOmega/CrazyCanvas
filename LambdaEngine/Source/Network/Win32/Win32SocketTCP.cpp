#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Network/Win32/Win32SocketTCP.h"
#include "Log/Log.h"
#include <winsock2.h>
#include <Ws2tcpip.h>

namespace LambdaEngine
{
	Win32SocketTCP::Win32SocketTCP() : Win32SocketTCP(INVALID_SOCKET, "", 0)
	{
		m_Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		if (m_Socket == INVALID_SOCKET)
		{
			LOG_ERROR_CRIT("Failed to create TCP socket");
			PrintLastError();
		}
	}

	Win32SocketTCP::Win32SocketTCP(uint64 socket, const char* address, uint16 port) : Win32SocketBase()
	{
		m_Socket = socket;
		m_Address = address;
		m_Port = port;
	}

	bool Win32SocketTCP::Connect(const std::string& address, uint16 port)
	{
		struct sockaddr_in socketAddress;
		socketAddress.sin_family = AF_INET;
		inet_pton(AF_INET, address.c_str(), &socketAddress.sin_addr.s_addr);
		socketAddress.sin_port = htons(port);

		m_Port = port;
		m_Address = address;

		if (connect(m_Socket, (struct sockaddr*)&socketAddress, sizeof(sockaddr_in)) == SOCKET_ERROR)
		{
			LOG_ERROR_CRIT("Failed to connect to %s:%d", address.c_str(), port);
			PrintLastError();
			return false;
		}
		return true;
	}

	bool Win32SocketTCP::Listen()
	{
		int32 result = listen(m_Socket, 64);
		if (result == SOCKET_ERROR)
		{
			LOG_ERROR_CRIT("Failed to listen");
			PrintLastError();
			return false;
		}
		return true;
	}

	ISocketTCP* Win32SocketTCP::Accept()
	{
		struct sockaddr_in socketAddress;
		int32 size = sizeof(struct sockaddr_in);
		SOCKET socket = accept(m_Socket, (struct sockaddr*)&socketAddress, &size);

		if (socket == INVALID_SOCKET)
		{
			LOG_ERROR_CRIT("Failed to accept Socket");
			PrintLastError();
			return nullptr;
		}

		char* address = inet_ntoa(socketAddress.sin_addr);
		uint16 port = ntohs(socketAddress.sin_port);

		return new Win32SocketTCP(socket, address, port);
	}

	bool Win32SocketTCP::Send(const char* buffer, uint32 bytesToSend, int32& bytesSent)
	{
		bytesSent = send(m_Socket, buffer, bytesToSend, 0);
		if (bytesSent == SOCKET_ERROR)
		{
			LOG_ERROR_CRIT("Failed to send data");
			PrintLastError();
			return false;
		}
		return true;
	}

	bool Win32SocketTCP::Receive(char* buffer, uint32 size, int32& bytesReceived)
	{
		bytesReceived = recv(m_Socket, buffer, size, 0);
		if (bytesReceived == SOCKET_ERROR)
		{
			bytesReceived = 0;
			int32 error = WSAGetLastError();
			if (IsClosed())
				return true;
			else if (error == WSAEWOULDBLOCK && IsNonBlocking())
				return true;

			LOG_ERROR_CRIT("Failed to receive data");
			PrintLastError();
			return false;
		}
		return true;
	}

	const std::string& Win32SocketTCP::GetAddress()
	{
		return m_Address;
	}

	uint16 Win32SocketTCP::GetPort()
	{
		return m_Port;
	}
}
#endif
