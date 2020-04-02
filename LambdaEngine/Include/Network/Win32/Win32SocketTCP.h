#pragma once

#include "Win32SocketBase.h"
#include "../API/ISocketTCP.h"

namespace LambdaEngine
{
	class LAMBDA_API Win32SocketTCP : public Win32SocketBase<ISocketTCP>
	{	
		friend class Win32SocketFactory;

	public:
		virtual bool Connect(const std::string& address, uint16 port) override;
		virtual bool Listen() override;
		virtual ISocketTCP* Accept() override;
		virtual bool Send(const char* buffer, uint32 bytesToSend, int32& bytesSent) override;
		virtual bool Receive(char* buffer, uint32 size, int32& bytesReceived) override;

		virtual const std::string& GetAddress() override;
		virtual uint16 GetPort() override;

	private:
		Win32SocketTCP();
		Win32SocketTCP(uint64 m_Socket, const char* address, uint16 port);

	private:
		std::string m_Address;
		uint16 m_Port;
	};
}
