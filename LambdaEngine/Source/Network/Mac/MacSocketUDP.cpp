#ifdef LAMBDA_PLATFORM_MACOS
#include "Network/Mac/MacSocketUDP.h"

#include "Log/Log.h"

#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace LambdaEngine
{
	MacSocketUDP::MacSocketUDP()
        : MacSocketBase<ISocketUDP>()
	{
		m_Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		if (m_Socket == INVALID_SOCKET)
		{
            int32 error = errno;
			LOG_ERROR_CRIT("Failed to create UDP socket");
			PrintLastError(error);
		}
	}

	bool MacSocketUDP::SendTo(const char* buffer, uint32 bytesToSend, int32& bytesSent, const std::string& address, uint16 port)
	{
		struct sockaddr_in socketAddress;
		socketAddress.sin_family = AF_INET;
		socketAddress.sin_port = htons(port);

		if (address.empty() || address == ADDRESS_ANY)
			socketAddress.sin_addr.s_addr = htonl(INADDR_ANY);
		else if (address == ADDRESS_LOOPBACK)
			socketAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		else if (address == ADDRESS_BROADCAST)
			socketAddress.sin_addr.s_addr = htonl(INADDR_BROADCAST);
		else
			inet_pton(AF_INET, address.c_str(), &socketAddress.sin_addr.s_addr);

		bytesSent = sendto(m_Socket, buffer, bytesToSend, 0, (struct sockaddr*)&socketAddress, sizeof(struct sockaddr_in));
		if (bytesSent == SOCKET_ERROR)
		{
            int32 error = errno;
            LOG_ERROR_CRIT("Failed to send data to %s:%d", address.c_str(), port);
			PrintLastError(error);
			return false;
		}
		return true;
	}

	bool MacSocketUDP::ReceiveFrom(char* buffer, uint32 size, int32& bytesReceived, std::string& address, uint16& port)
	{
		struct sockaddr_in socketAddress;
		socklen_t socketAddressSize = sizeof(struct sockaddr_in);

		bytesReceived = recvfrom(m_Socket, buffer, size, 0, (struct sockaddr*)&socketAddress, &socketAddressSize);
		if (bytesReceived == SOCKET_ERROR)
		{
            int32 error = errno;
			LOG_ERROR_CRIT("Failed to receive data from");
			PrintLastError(error);
			return false;
		}

		address = inet_ntoa(socketAddress.sin_addr);
		port = ntohs(socketAddress.sin_port);

		return true;
	}

	bool MacSocketUDP::EnableBroadcast()
	{
		static const int broadcast = 1;
		if (setsockopt(m_Socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) == SOCKET_ERROR)
		{
            int32 error = errno;
			LOG_ERROR_CRIT("Failed to enable Broadcast");
			PrintLastError(error);
			return false;
		}
        
		return true;
	}

	bool MacSocketUDP::Broadcast(const char* buffer, uint32 bytesToSend, int32& bytesSent, uint16 port)
	{
		return SendTo(buffer, bytesToSend, bytesSent, "", port);
	}
}
#endif
