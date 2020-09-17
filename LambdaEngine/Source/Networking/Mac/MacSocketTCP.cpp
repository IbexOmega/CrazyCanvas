#ifdef LAMBDA_PLATFORM_MACOS
#include "Networking/Mac/MacSocketTCP.h"

#include <sys/types.h> 
#include <sys/socket.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <arpa/inet.h>

namespace LambdaEngine
{
    MacSocketTCP::MacSocketTCP()
    {
        m_Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_Socket == INVALID_SOCKET)
        {
            int32 error = errno;
            LOG_ERROR_CRIT("Failed to create TCP socket");
            PrintLastError(error);
        }
    }

    MacSocketTCP::MacSocketTCP(int32 socket, const char* pAddress, uint16 port)
        : MacSocketBase<ISocketTCP>(socket),
        m_Address(pAddress),
        m_Port(port)
    {
    }

    bool MacSocketTCP::Listen()
    {
        int32 result = listen(m_Socket, 64);
		if (result == SOCKET_ERROR)
		{
            int32 error = errno;
			LOG_ERROR_CRIT("Failed to listen");
			PrintLastError(error);
			return false;
		}
		return true;
    }

    ISocketTCP* MacSocketTCP::Accept()
    {
        struct sockaddr_in socketAddress;
		socklen_t size = sizeof(struct sockaddr_in);
		int32 socket = accept(m_Socket, (struct sockaddr*)&socketAddress, &size);

		if (socket == INVALID_SOCKET)
		{
			int32 error = errno;
			if (error != WSAEINTR)
			{
				LOG_ERROR_CRIT("Failed to accept Socket");
				PrintLastError(error);
			}
			return nullptr;
		}

		char* address = inet_ntoa(socketAddress.sin_addr);
		uint16 port = ntohs(socketAddress.sin_port);

        return DBG_NEW MacSocketTCP(socket, address, port);
    }

    bool MacSocketTCP::Send(const uint8* pBuffer, uint32 bytesToSend, int32& bytesSent)
    {
        bytesSent = send(m_Socket, (const char*)pBuffer, bytesToSend, 0);
		if (bytesSent == SOCKET_ERROR)
		{
            int32 error = errno;
			LOG_ERROR_CRIT("Failed to send data");
			PrintLastError(error);
			return false;
		}
		return true;
    }

    bool MacSocketTCP::Receive(uint8* pBuffer, uint32 size, int32& bytesReceived)
    {
		bytesReceived = recv(m_Socket, (char*)pBuffer, size, 0);
		if (bytesReceived == SOCKET_ERROR)
		{
			bytesReceived = 0;
			int32 error = errno;

			if (IsClosed())
				return true;
			else if (error == WSAEWOULDBLOCK && IsNonBlocking())
				return true;
			else if (error == WSAECONNRESET || error == WSAECONNABORTED)
				return false;

			LOG_ERROR_CRIT("Failed to receive data");
			PrintLastError(error);
			return false;
		}
        else if (bytesReceived == 0)
        {
            return false;
        }
        
		return true;
	}

    bool MacSocketTCP::EnableNaglesAlgorithm(bool enable)
	{
		static const int broadcast = enable ? 1 : 0;
		if (setsockopt(m_Socket, IPPROTO_TCP, TCP_NODELAY, &broadcast, sizeof(broadcast)) == SOCKET_ERROR)
		{
            int32 error = errno;
            
			LOG_ERROR_CRIT("Failed to set socket option Nagle's Algorithm (TCP_NODELAY), [Enable=%b]", enable);
			PrintLastError(error);
			return false;
		}
        
		return true;
	}
}
#endif
