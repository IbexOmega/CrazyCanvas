#pragma once

#include "ISocket.h"

namespace LambdaEngine
{
	class ISocketUDP : public ISocket
	{
	public:
		DECL_INTERFACE(ISocketUDP);

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
		virtual bool SendTo(const uint8* pBuffer, uint32 bytesToSend, int32& bytesSent, const IPEndPoint& ipEndPoint) = 0;

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
		virtual bool ReceiveFrom(uint8* pBuffer, uint32 size, int32& bytesReceived, IPEndPoint& ipEndPoint) = 0;

		/*
		* Enables or disables the broadcast functionality
		*
		* return	  - False if an error occured, otherwise true.
		*/
		virtual bool EnableBroadcast(bool enable) = 0;
	};
}
