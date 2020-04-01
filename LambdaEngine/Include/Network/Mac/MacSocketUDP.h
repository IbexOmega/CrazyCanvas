#pragma oncde

//#ifdef LAMBDA_PLATFORM_MACOS

#include "Network/API/ISocketUDP.h"
#include "MacSocketBase.h"

namespace LambdaEngine
{
    class MacSocketUDP : public MacSocketBase<ISocketUDP>
    {
        friend class MacSocketFactory;
        
    public:
		virtual bool SendTo(const char* buffer, uint32 bytesToSend, uint32& bytesSent, const std::string& address, uint16 port) override;
		virtual bool ReceiveFrom(char* buffer, uint32 size, uint32& bytesReceived, std::string& address, uint16& port) override;
		virtual bool EnableBroadcast() override;
		virtual bool Broadcast(const char* buffer, uint32 bytesToSend, uint32& bytesSent, uint16 port) override;

	private:
		MacSocketUDP();
    };
}
//#endif
