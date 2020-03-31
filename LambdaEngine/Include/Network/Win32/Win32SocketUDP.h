#pragma once

#include "Win32Socket.h"
#include "../API/ISocketUDP.h"

namespace LambdaEngine
{
	class LAMBDA_API Win32SocketUDP : public Win32Socket<ISocketUDP>
	{	
		friend class Win32SocketFactory;

	public:
		virtual bool SendTo(const char* buffer, uint32 bytesToSend, uint32& bytesSent, const std::string& address, uint16 port) override;
		virtual bool ReceiveFrom(char* buffer, uint32 size, uint32& bytesReceived, std::string& address, uint16& port) override;

	private:
		Win32SocketUDP();
	};
}
