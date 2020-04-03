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
            {
                inet_pton(AF_INET, address.c_str(), &socketAddress.sin_addr.s_addr);
            }
			else
            {
                socketAddress.sin_addr.s_addr = htonl(INADDR_ANY);
            }

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
            u_long tempNonBlocking = (u_long)nonBlocking;
			if (ioctl(m_Socket, FIONBIO, &tempNonBlocking) == SOCKET_ERROR)
			{
				LOG_ERROR_CRIT("Failed to change blocking mode to [%sBlocking] ", nonBlocking ? "Non " : "");
				//PrintLastError();
				return false;
			}
            
            m_NonBlocking = nonBlocking;
            return true;
		};

        virtual bool IsNonBlocking() const override
        {
            return m_NonBlocking;
        }
                
		virtual bool Close() override
		{
            if (m_Closed)
            {
                return true;
            }

            m_Closed = true;

            if (close(m_Socket) == SOCKET_ERROR)
            {
                LOG_ERROR_CRIT("Failed to close socket");
                //PrintLastError();
                return false;
            }
                
            return true;
		}
                
        virtual bool IsClosed() const override
        {
            return m_Closed;
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
		int32   m_Socket      = INVALID_SOCKET;
        bool    m_NonBlocking = false;
        bool    m_Closed      = false;
	};
}

#endif
