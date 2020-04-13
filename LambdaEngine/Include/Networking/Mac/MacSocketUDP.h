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
		/*
		* Sends a buffer of data to the specified address and port
		*
		* pBuffer	  - The buffer to send.
		* bytesToSend - The number of bytes to send.
		* bytesSent	  - Will return the number of bytes actually sent.
		* address	  - The inet address to send to.
		* port		  - The port.
		*
		* return	  - False if an error occured, otherwise true.
		*/
		virtual bool SendTo(const char* pBuffer, uint32 bytesToSend, int32& bytesSent, const std::string& address, uint16 port) override;

		/*
		* Receives a buffer of data.
		*
		* pBuffer	  - The buffer to read into.
		* bytesToRead - The number of bytes to read.
		* bytesRead	  - Will return the number of bytes actually read.
		* address	  - Will return the inet address the data came from.
		* port		  - Will return the port the data came from.
		*
		* return	  - False if an error occured, otherwise true.
		*/
		virtual bool ReceiveFrom(char* pBuffer, uint32 size, int32& bytesReceived, std::string& address, uint16& port) override;

		/*
		* Enables the broadcast functionality
		*
		* enable	- True to enable broadcast, false to disable broadcast
		*
		* return	- False if an error occured, otherwise true.
		*/
		virtual bool EnableBroadcast(bool enable);

		/*
		* Sends a buffer of data to the broadcast address
		*
		* buffer	  - The buffer to send.
		* bytesToSend - The number of bytes to send.
		* bytesSent	  - Will return the number of bytes actually sent.
		* port		  - The port.
		*
		* return	  - False if an error occured, otherwise true.
		*/
		virtual bool Broadcast(const char* pBuffer, uint32 bytesToSend, int32& bytesSent, uint16 port) override;

	private:
		MacSocketUDP();
    };
}
//#endif
