#pragma once

#include "../API/ISocket.h"
#include "Types.h"

namespace LambdaEngine
{
	class LAMBDA_API Win32Socket : public ISocket
	{	
		friend class Win32SocketFactory;

	public:
		virtual bool Connect(const std::string& address, uint16 port) override;
		virtual bool Bind(const std::string& address, uint16 port) override;
		virtual bool Listen() override;
		virtual ISocket* Accept() override;
		virtual bool Send(const char* buffer, uint32 bytesToSend, uint32& bytesSent) override;
		virtual bool Receive(char* buffer, uint32 size, uint32& bytesReceived) override;
		virtual bool SendTo(const char* buffer, uint32 bytesToSend, uint32& bytesSent, const std::string& address, uint16 port) override;
		virtual bool ReceiveFrom(char* buffer, uint32 size, uint32& bytesReceived, std::string& address, uint16& port) override;
		virtual void Close() override;

		virtual const std::string& GetAddress() override;
		virtual uint16 GetPort() override;

	protected:
		virtual bool Init(EProtocol protocol) override;

	private:
		Win32Socket();
		Win32Socket(uint64 m_Socket, const char* address, uint16 port);

		void PrintLastError() const;

	private:
		uint64 m_Socket;
		std::string m_Address;
		uint16 m_Port;
	};
}
