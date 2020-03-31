#pragma once

#include "Defines.h"
#include "EProtocol.h"
#include "Types.h"
#include <string>

namespace LambdaEngine
{
	class LAMBDA_API ISocket
	{
	public:
		DECL_INTERFACE(ISocket);

		virtual bool Connect(const char* address, uint16 port) = 0;
		virtual bool Bind(const char* address, uint16 port) = 0;
		virtual bool Listen() = 0;
		virtual ISocket* Accept() = 0;
		virtual bool Send(const char* buffer, uint32 bytesToSend, uint32& bytesSent) = 0;
		virtual bool Receive(char* buffer, uint32 size, uint32& bytesReceived) = 0;
		virtual void Close() = 0;

	protected:
		virtual bool Init(EProtocol protocol) = 0;
	};
}
