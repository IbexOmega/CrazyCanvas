#pragma once

#include "ISocket.h"

namespace LambdaEngine
{
	class LAMBDA_API ISocketTCP : public ISocket
	{
	public:
		DECL_INTERFACE(ISocketTCP);

		virtual bool Connect(const std::string& address, uint16 port) = 0;
		virtual bool Listen() = 0;
		virtual ISocketTCP* Accept() = 0;
		virtual bool Send(const char* buffer, uint32 bytesToSend, int32& bytesSent) = 0;
		virtual bool Receive(char* buffer, uint32 size, int32& bytesReceived) = 0;

		virtual const std::string& GetAddress() = 0;
		virtual uint16 GetPort() = 0;
	};
}
