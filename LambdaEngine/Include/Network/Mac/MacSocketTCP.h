#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#include "Network/API/ISocketTCP.h"

#include "MacSocketBase.h"

namespace LambdaEngine
{
    class MacSocketTCP : public MacSocketBase<ISocketTCP>
    {
        friend class MacSocketFactory;
        
    public:
        ~MacSocketTCP() = default;

        virtual bool Connect(const std::string& address, uint16 port) override;
		virtual bool Listen() override;
		virtual ISocketTCP* Accept() override;
		virtual bool Send(const char* buffer, uint32 bytesToSend, int32& bytesSent) override;
		virtual bool Receive(char* buffer, uint32 size, int32& bytesReceived) override;

		virtual const std::string& GetAddress() override;
		virtual uint16 GetPort() override;

    private:
        MacSocketTCP();
		MacSocketTCP(int32 m_Socket, const char* address, uint16 port);

    private:
		std::string m_Address;
		uint16 m_Port;
    };
}

#endif
