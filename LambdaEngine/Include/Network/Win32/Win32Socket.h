#pragma once

#include "../API/ISocket.h"
#include "Types.h"

namespace LambdaEngine
{
	class LAMBDA_API Win32Socket : public ISocket
	{	
		friend class Win32SocketFactory;

	public:
		virtual bool Connect(const char* address, uint16 port) override;
		virtual bool Bind(const char* address, uint16 port) override;
		virtual bool Listen() override;
		virtual ISocket* Accept() override;
		virtual bool Send(const char* buffer, uint32 bytesToSend, uint32& bytesSent) override;
		virtual bool Receive(char* buffer, uint32 size, uint32& bytesReceived) override;
		virtual void Close() override;

	protected:
		virtual bool Init(EProtocol protocol) override;

	private:
		Win32Socket();
		Win32Socket(uint64 m_Socket);

		void PrintLastError() const;

	private:
		uint64 m_Socket;
	};
}
