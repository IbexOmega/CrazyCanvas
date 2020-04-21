#ifdef LAMBDA_PLATFORM_MACOS
#include "Networking/Mac/MacSocketUDP.h"

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

	bool MacSocketUDP::SendTo(const char* pBuffer, uint32 bytesToSend, int32& bytesSent, const IPEndPoint& pIPEndPoint)
	{
        struct sockaddr_in socketAddress;
        IPEndPointToSocketAddress(&pIPEndPoint, &socketAddress);

		bytesSent = sendto(m_Socket, pBuffer, bytesToSend, 0, (struct sockaddr*)&socketAddress, sizeof(struct sockaddr_in));
		if (bytesSent == SOCKET_ERROR)
		{
            int32 error = errno;
            LOG_ERROR_CRIT("Failed to send data to %s", pIPEndPoint.ToString().c_str());
			PrintLastError(error);
			return false;
		}
		return true;
	}

	bool MacSocketUDP::ReceiveFrom(char* pBuffer, uint32 size, int32& bytesReceived, IPEndPoint& pIPEndPoint)
	{
		struct sockaddr_in socketAddress;
		socklen_t socketAddressSize = sizeof(struct sockaddr_in);

		bytesReceived = recvfrom(m_Socket, pBuffer, size, 0, (struct sockaddr*)&socketAddress, &socketAddressSize);
		if (bytesReceived == SOCKET_ERROR)
		{
			int32 error = errno;

			if (IsClosed() && !IsNonBlocking())
				return false;
			else if (error == ECONNREFUSED)
				return true;

			LOG_ERROR_CRIT("Failed to receive data from");
			PrintLastError(error);
			return false;
		}

        inet_ntop(socketAddress.sin_family, &socketAddress.sin_addr, m_pReceiveAddressBuffer, s_ReceiveAddressBufferSize);
        uint16 port = ntohs(socketAddress.sin_port);

        pIPEndPoint.SetEndPoint(IPAddress::Get(m_pReceiveAddressBuffer), port);

		return true;
	}

	bool MacSocketUDP::EnableBroadcast(bool enable)
	{
		static const int broadcast = enable ? 1 : 0;
		if (setsockopt(m_Socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) == SOCKET_ERROR)
		{
            int32 error = errno;
			LOG_ERROR_CRIT("Failed to set Broadcast option [Enable=%b]", enable);
			PrintLastError(error);
			return false;
		}
        
		return true;
	}
}
#endif
