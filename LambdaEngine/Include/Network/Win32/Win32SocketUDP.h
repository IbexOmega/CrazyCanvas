#pragma once

#include "Win32SocketBase.h"
#include "../API/ISocketUDP.h"

namespace LambdaEngine
{
	class LAMBDA_API Win32SocketUDP : public Win32SocketBase<ISocketUDP>
	{	
		friend class Win32SocketFactory;

	public:
		virtual bool SendTo(const char* buffer, uint32 bytesToSend, int32& bytesSent, const std::string& address, uint16 port) override;
		virtual bool ReceiveFrom(char* buffer, uint32 size, int32& bytesReceived, std::string& address, uint16& port) override;
		virtual bool EnableBroadcast() override;
		virtual bool Broadcast(const char* buffer, uint32 bytesToSend, int32& bytesSent, uint16 port) override;

	private:
		Win32SocketUDP();
	};
}
