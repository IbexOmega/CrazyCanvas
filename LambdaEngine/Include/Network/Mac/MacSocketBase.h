#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#include "Types.h"
#include "Log/Log.h"

#include <string>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/ioctl.h>

#include <netinet/in.h>
#include <arpa/inet.h>


#define INVALID_SOCKET  -1
#define SOCKET_ERROR    -1

namespace LambdaEngine
{
	template <typename IBase>
	class MacSocketBase : public IBase
	{	
	public:
		bool Bind(const std::string& address, uint16 port) override
		{
			struct sockaddr_in socketAddress;
			socketAddress.sin_family = AF_INET;

			if (!address.empty())
				inet_pton(AF_INET, address.c_str(), &socketAddress.sin_addr.s_addr);
			else
				socketAddress.sin_addr.s_addr = htonl(INADDR_ANY);

			socketAddress.sin_port = htons(port);

            
            if (bind(m_Socket, (struct sockaddr*)&socketAddress, sizeof(sockaddr_in)) == SOCKET_ERROR)
            {
				LOG_ERROR_CRIT("Failed to bind to %s:%d", !address.empty() ? address.c_str() : "ANY", port);
				//PrintLastError();
				return false;
			}
			return true;
		}

		virtual bool SetNonBlocking(bool nonBlocking) override
		{
			if (ioctl(m_Socket, FIONBIO, &((u_long)nonBlocking) != NO_ERROR)
			{
				LOG_ERROR_CRIT("Failed to change blocking mode to [%sBlocking] ", nonBlocking ? "Non " : "");
				//PrintLastError();
				return false;
			}
			return true;
		};

		void Close() override
		{
			close(m_Socket);
		}

	protected:
        MacSocketBase(int32 socket = INVALID_SOCKET)
            : m_Socket(socket)
		{
		}
        
       ~MacSocketBase()
        {
            Close();
        }

	protected:
		int32 m_Socket;
	};
}

#endif
