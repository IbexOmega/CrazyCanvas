#pragma once
#include "Win32SocketBase.h"
#include "Networking/API/ISocketUDP.h"

namespace LambdaEngine
{
	class Win32SocketUDP : public Win32SocketBase<ISocketUDP>
	{	
		friend class Win32NetworkUtils;

	public:
		/*
		* Sends a buffer of data to the specified address and port
		*
		* pBuffer	  - The buffer to send.
		* bytesToSend - The number of bytes to send.
		* bytesSent	  - Will return the number of bytes actually sent.
		* ipEndPoint  - The IPEndPoint to send the datagram packet to
		*
		* return	  - False if an error occured, otherwise true.
		*/
		virtual bool SendTo(const char* pBuffer, uint32 bytesToSend, int32& bytesSent, const IPEndPoint& ipEndPoint) override;

		/*
		* Receives a buffer of data.
		*
		* pBuffer	  - The buffer to read into.
		* bytesToRead - The number of bytes to read.
		* bytesRead	  - Will return the number of bytes actually read.
		* ipEndPoint  - Will return the IPEndPoint the datagram packet came from
		*
		* return	  - False if an error occured, otherwise true.
		*/
		virtual bool ReceiveFrom(char* pBuffer, uint32 size, int32& bytesReceived, IPEndPoint& ipEndPoint) override;

		/*
		* Enables the broadcast functionality
		*
		* enable	- True to enable broadcast, false to disable broadcast
		*
		* return	- False if an error occured, otherwise true.
		*/
		virtual bool EnableBroadcast(bool enable) override;

	private:
		Win32SocketUDP();
	};
}
