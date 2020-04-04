#pragma once

#include "ISocket.h"

namespace LambdaEngine
{
	class LAMBDA_API ISocketTCP : public ISocket
	{
	public:
		DECL_INTERFACE(ISocketTCP);

		/*
		* Sets the socket in listening mode to listen for incoming connections.
		*
		* return  - False if an error occured, otherwise true.
		*/
		virtual bool Listen() = 0;

		/*
		* Accepts an incoming connection and creates a socket for further comunication
		*
		* return  - nullptr if an error occured, otherwise a ISocketTCP*.
		*/
		virtual ISocketTCP* Accept() = 0;

		/*
		* Sends a buffer of data
		*
		* buffer	  - The buffer to send.
		* bytesToSend - The number of bytes to send.
		* bytesSent	  - Will return the number of bytes actually sent.
		*
		* return	  - False if an error occured, otherwise true.
		*/
		virtual bool Send(const char* buffer, uint32 bytesToSend, int32& bytesSent) = 0;

		/*
		* Receives a buffer of data.
		*
		* buffer	  - The buffer to read into.
		* bytesToRead - The number of bytes to read.
		* bytesRead	  - Will return the number of bytes actually read.
		*
		* return	  - False if an error occured, otherwise true. 
		*/
		virtual bool Receive(char* buffer, uint32 bytesToRead, int32& bytesRead) = 0;

		/*
		* Disables Nagle's Algorithm, commonly known as TCP_NODELAY
		*
		* return	  - False if an error occured, otherwise true.
		*/
		virtual bool DisableNaglesAlgorithm() = 0;
	};
}
