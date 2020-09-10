#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Networking/Win32/Win32SocketTCP.h"
#include "Log/Log.h"
#include <winsock2.h>
#include <Ws2tcpip.h>

namespace LambdaEngine
{
	Win32SocketTCP::Win32SocketTCP() : Win32SocketBase()
	{
		m_Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		if (m_Socket == INVALID_SOCKET)
		{
			LOG_ERROR_CRIT("Failed to create TCP socket");
			PrintLastError();
		}
	}

	Win32SocketTCP::Win32SocketTCP(uint64 socket, const IPEndPoint& pIPEndPoint) : Win32SocketBase(socket, pIPEndPoint)
	{

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
			int32 error = WSAGetLastError();
			if (error != WSAEINTR)
			{
				LOG_ERROR_CRIT("Failed to accept Socket");
				PrintLastError();
			}
			return nullptr;
		}

		inet_ntop(socketAddress.sin_family, &socketAddress.sin_addr, m_pReceiveAddressBuffer, s_ReceiveAddressBufferSize);
		uint16 port = ntohs(socketAddress.sin_port);

		IPEndPoint endPoint(IPAddress::Get(m_pReceiveAddressBuffer), port);
		return DBG_NEW Win32SocketTCP(socket, endPoint);
	}

	bool Win32SocketTCP::Send(const uint8* pBuffer, uint32 bytesToSend, int32& bytesSent)
	{
		bytesSent = send(m_Socket, (const char*)pBuffer, bytesToSend, 0);
		if (bytesSent == SOCKET_ERROR)
		{
			LOG_ERROR_CRIT("Failed to send data");
			PrintLastError();
			return false;
		}
		return true;
	}

	bool Win32SocketTCP::Receive(uint8* pBuffer, uint32 size, int32& bytesReceived)
	{
		bytesReceived = recv(m_Socket, (char*)pBuffer, size, 0);
		if (bytesReceived == SOCKET_ERROR)
		{
			if (IsClosed() && !IsNonBlocking())
				return false;

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

	bool Win32SocketTCP::EnableNaglesAlgorithm(bool enable)
	{
		static const char broadcast = enable ? 1 : 0;
		if (setsockopt(m_Socket, IPPROTO_TCP, TCP_NODELAY, &broadcast, sizeof(broadcast)) == SOCKET_ERROR)
		{
			LOG_ERROR_CRIT("Failed to set socket option Nagle's Algorithm (TCP_NODELAY), [Enable=%b]", enable);
			PrintLastError();
			return false;
		}
		return true;
	}
}
#endif
