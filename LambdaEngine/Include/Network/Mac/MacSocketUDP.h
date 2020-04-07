#pragma oncde

//#ifdef LAMBDA_PLATFORM_MACOS

#include "Network/API/UDP/ISocketUDP.h"
#include "MacSocketBase.h"

namespace LambdaEngine
{
    class MacSocketUDP : public MacSocketBase<ISocketUDP>
    {
        friend class MacNetworkUtils;
        
    public:
		virtual bool SendTo(const char* buffer, uint32 bytesToSend, int32& bytesSent, const std::string& address, uint16 port) override;
		virtual bool ReceiveFrom(char* buffer, uint32 size, int32& bytesReceived, std::string& address, uint16& port) override;
		virtual bool EnableBroadcast() override;
		virtual bool Broadcast(const char* buffer, uint32 bytesToSend, int32& bytesSent, uint16 port) override;

	private:
		MacSocketUDP();
    };
}
//#endif
