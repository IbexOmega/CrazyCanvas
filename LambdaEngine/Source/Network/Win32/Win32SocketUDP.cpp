#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Network/Win32/Win32SocketUDP.h"
#include "Log/Log.h"
#include <winsock2.h>
#include <Ws2tcpip.h>

namespace LambdaEngine
{
	Win32SocketUDP::Win32SocketUDP() : Win32SocketBase()
	{
		m_Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		if (m_Socket == INVALID_SOCKET)
		{
			LOG_ERROR_CRIT("Failed to create UDP socket");
			PrintLastError();
		}
	}

	bool Win32SocketUDP::SendTo(const char* pBuffer, uint32 bytesToSend, int32& bytesSent, const std::string& address, uint16 port)
	{
		struct sockaddr_in socketAddress;
		socketAddress.sin_family = AF_INET;
		socketAddress.sin_port = htons(port);

		if (address.empty() || address == ADDRESS_ANY)
			socketAddress.sin_addr.s_addr = INADDR_ANY;
		else if (address == ADDRESS_LOOPBACK)
			socketAddress.sin_addr.s_addr = INADDR_LOOPBACK;
		else if (address == ADDRESS_BROADCAST)
			socketAddress.sin_addr.s_addr = INADDR_BROADCAST;
		else
			inet_pton(AF_INET, address.c_str(), &socketAddress.sin_addr.s_addr);

		bytesSent = sendto(m_Socket, pBuffer, bytesToSend, 0, (struct sockaddr*)&socketAddress, sizeof(struct sockaddr_in));
		if (bytesSent == SOCKET_ERROR)
		{
			LOG_ERROR_CRIT("Failed to send data to %s:%d", address.c_str(), port);
			PrintLastError();
			return false;
		}
		return true;
	}

	bool Win32SocketUDP::ReceiveFrom(char* pBuffer, uint32 size, int32& bytesReceived, std::string& address, uint16& port)
	{
		struct sockaddr_in socketAddress;
		int32 socketAddressSize = sizeof(struct sockaddr_in);

		bytesReceived = recvfrom(m_Socket, pBuffer, size, 0, (struct sockaddr*)&socketAddress, &socketAddressSize);
		if (bytesReceived == SOCKET_ERROR)
		{
			LOG_ERROR_CRIT("Failed to receive data from");
			PrintLastError();
			return false;
		}

		address.resize(16);
		inet_ntop(socketAddress.sin_family, &socketAddress.sin_addr, address.data(), address.length());
		port = ntohs(socketAddress.sin_port);

		return true;
	}

	bool Win32SocketUDP::EnableBroadcast(bool enable)
	{
		static const char broadcast = enable ? 1 : 0;
		if (setsockopt(m_Socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) == SOCKET_ERROR)
		{
			LOG_ERROR_CRIT("Failed to set Broadcast option [Enable=%b]", enable);
			PrintLastError();
			return false;
		}
		return true;
	}

	bool Win32SocketUDP::Broadcast(const char* pBuffer, uint32 bytesToSend, int32& bytesSent, uint16 port)
	{
		return SendTo(pBuffer, bytesToSend, bytesSent, "", port);
	}
}
#endif
