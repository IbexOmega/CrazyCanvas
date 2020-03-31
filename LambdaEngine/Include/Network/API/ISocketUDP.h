#pragma once

#include "ISocket.h"

namespace LambdaEngine
{
	class LAMBDA_API ISocketUDP : public ISocket
	{
	public:
		DECL_INTERFACE(ISocketUDP);

		virtual bool SendTo(const char* buffer, uint32 bytesToSend, uint32& bytesSent, const std::string& address, uint16 port) = 0;
		virtual bool ReceiveFrom(char* buffer, uint32 size, uint32& bytesReceived, std::string& address, uint16& port) = 0;
		virtual bool EnableBroadcast() = 0;
		virtual bool Broadcast(const char* buffer, uint32 bytesToSend, uint32& bytesSent, uint16 port) = 0;
	};
}
