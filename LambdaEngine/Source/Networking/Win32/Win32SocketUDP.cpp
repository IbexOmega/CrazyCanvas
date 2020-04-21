#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Networking/Win32/Win32SocketUDP.h"
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

	bool Win32SocketUDP::SendTo(const char* pBuffer, uint32 bytesToSend, int32& bytesSent, const IPEndPoint& pIPEndPoint)
	{
		struct sockaddr_in socketAddress;
		IPEndPointToSocketAddress(&pIPEndPoint, &socketAddress);

		bytesSent = sendto(m_Socket, pBuffer, bytesToSend, 0, (struct sockaddr*)&socketAddress, sizeof(struct sockaddr_in));
		if (bytesSent == SOCKET_ERROR)
		{
			LOG_ERROR_CRIT("Failed to send datagram packet to %s", pIPEndPoint.ToString().c_str());
			PrintLastError();
			return false;
		}
		return true;
	}

	bool Win32SocketUDP::ReceiveFrom(char* pBuffer, uint32 size, int32& bytesReceived, IPEndPoint& pIPEndPoint)
	{
		struct sockaddr_in socketAddress;
		int32 socketAddressSize = sizeof(struct sockaddr_in);

		bytesReceived = recvfrom(m_Socket, pBuffer, size, 0, (struct sockaddr*)&socketAddress, &socketAddressSize);
		if (bytesReceived == SOCKET_ERROR)
		{
			if (IsClosed() && !IsNonBlocking())
				return false;
			else if (WSAGetLastError() == WSAECONNRESET)
				return true;

			LOG_ERROR_CRIT("Failed to receive datagram packet");
			PrintLastError();
			return false;
		}

		inet_ntop(socketAddress.sin_family, &socketAddress.sin_addr, m_pReceiveAddressBuffer, s_ReceiveAddressBufferSize);
		uint16 port = ntohs(socketAddress.sin_port);

		pIPEndPoint.SetEndPoint(IPAddress::Get(m_pReceiveAddressBuffer), port);

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
}
#endif
