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
		* buffer	  - The buffer to send.
		* bytesToSend - The number of bytes to send.
		* bytesSent	  - Will return the number of bytes actually sent.
		* address	  - The inet address to send to.
		* port		  - The port.
		*
		* return	  - False if an error occured, otherwise true.
		*/
		virtual bool SendTo(const char* buffer, uint32 bytesToSend, int32& bytesSent, const std::string& address, uint16 port) = 0;

		/*
		* Receives a buffer of data.
		*
		* buffer	  - The buffer to read into.
		* bytesToRead - The number of bytes to read.
		* bytesRead	  - Will return the number of bytes actually read.
		* address	  - Will return the inet address the data came from.
		* port		  - Will return the port the data came from.
		*
		* return	  - False if an error occured, otherwise true.
		*/
		virtual bool ReceiveFrom(char* buffer, uint32 size, int32& bytesReceived, std::string& address, uint16& port) = 0;

		/*
		* Enables the broadcast functionality
		*
		* return	  - False if an error occured, otherwise true.
		*/
		virtual bool EnableBroadcast() = 0;

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
		virtual bool Broadcast(const char* buffer, uint32 bytesToSend, int32& bytesSent, uint16 port) = 0;
	};
}
