#pragma once

#include "Win32SocketBase.h"
#include "../API/ISocketTCP.h"

namespace LambdaEngine
{
	class Win32SocketTCP : public Win32SocketBase<ISocketTCP>
	{	
		friend class Win32SocketFactory;

	public:
		/*
		* Sets the socket in listening mode to listen for incoming connections.
		*
		* return  - False if an error occured, otherwise true.
		*/
		virtual bool Listen() override;

		/*
		* Accepts an incoming connection and creates a socket for further comunication
		*
		* return  - nullptr if an error occured, otherwise a ISocketTCP*.
		*/
		virtual ISocketTCP* Accept() override;

		/*
		* Sends a buffer of data
		*
		* buffer	  - The buffer to send.
		* bytesToSend - The number of bytes to send.
		* bytesSent	  - Will return the number of bytes actually sent.
		*
		* return	  - False if an error occured, otherwise true.
		*/
		virtual bool Send(const char* buffer, uint32 bytesToSend, int32& bytesSent) override;

		/*
		* Receives a buffer of data.
		*
		* buffer	  - The buffer to read into.
		* bytesToRead - The number of bytes to read.
		* bytesRead	  - Will return the number of bytes actually read.
		*
		* return	  - False if an error occured, otherwise true.
		*/
		virtual bool Receive(char* buffer, uint32 bytesToRead, int32& bytesRead) override;

		/*
		* Disables Nagle's Algorithm, commonly known as TCP_NODELAY
		*
		* return	  - False if an error occured, otherwise true.
		*/
		virtual bool DisableNaglesAlgorithm() override;

	private:
		Win32SocketTCP();
		Win32SocketTCP(uint64 m_Socket, const char* address, uint16 port);
	};
}
