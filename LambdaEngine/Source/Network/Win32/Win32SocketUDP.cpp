#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Network/Win32/Win32SocketUDP.h"
#include "Log/Log.h"
#include <winsock2.h>
#include <Ws2tcpip.h>

namespace LambdaEngine
{
	Win32SocketUDP::Win32SocketUDP() : Win32Socket()
	{
		m_Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		if (m_Socket == INVALID_SOCKET)
		{
			LOG_ERROR_CRIT("Failed to create UDP socket");
			PrintLastError();
		}
	}

	bool Win32SocketUDP::SendTo(const char* buffer, uint32 bytesToSend, uint32& bytesSent, const std::string& address, uint16 port)
	{
		struct sockaddr_in socketAddress;
		socketAddress.sin_family = AF_INET;
		inet_pton(AF_INET, address.c_str(), &socketAddress.sin_addr.s_addr);
		socketAddress.sin_port = htons(port);

		bytesSent = sendto(m_Socket, buffer, bytesToSend, 0, (struct sockaddr*)&socketAddress, sizeof(struct sockaddr_in));
		if (bytesSent == SOCKET_ERROR)
		{
			LOG_ERROR_CRIT("Failed to send data to %s:%d", address, port);
			PrintLastError();
			return false;
		}
		return true;
	}

	bool Win32SocketUDP::ReceiveFrom(char* buffer, uint32 size, uint32& bytesReceived, std::string& address, uint16& port)
	{
		struct sockaddr_in socketAddress;
		int32 socketAddressSize = sizeof(struct sockaddr_in);

		bytesReceived = recvfrom(m_Socket, buffer, size, 0, (struct sockaddr*)&socketAddress, &socketAddressSize);
		if (bytesReceived == SOCKET_ERROR)
		{
			LOG_ERROR_CRIT("Failed to receive data from");
			PrintLastError();
			return false;
		}

		address = inet_ntoa(socketAddress.sin_addr);
		port = ntohs(socketAddress.sin_port);

		return true;
	}
}
#endif
