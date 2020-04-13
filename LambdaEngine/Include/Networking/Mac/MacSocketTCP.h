#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#include "Networking/API/ISocketTCP.h"

#include "MacSocketBase.h"

namespace LambdaEngine
{
    class MacSocketTCP : public MacSocketBase<ISocketTCP>
    {
        friend class MacNetworkUtils;
        
    public:
        ~MacSocketTCP() = default;

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
		* pBuffer	  - The buffer to send.
		* bytesToSend - The number of bytes to send.
		* bytesSent	  - Will return the number of bytes actually sent.
		*
		* return	  - False if an error occured, otherwise true.
		*/
		virtual bool Send(const char* pBuffer, uint32 bytesToSend, int32& bytesSent) override;

		/*
		* Receives a buffer of data.
		*
		* pBuffer	  - The buffer to read into.
		* bytesToRead - The number of bytes to read.
		* bytesRead	  - Will return the number of bytes actually read.
		*
		* return	  - False if an error occured, otherwise true.
		*/
		virtual bool Receive(char* pBuffer, uint32 bytesToRead, int32& bytesRead) override;

		/*
		* Enables or Disables Nagle's Algorithm, commonly known as TCP_NODELAY
		*
		* enable	- True to enable, false to disable
		*
		* return	- False if an error occured, otherwise true.
		*/
		virtual bool EnableNaglesAlgorithm(bool enable) override;

    private:
        MacSocketTCP();
		MacSocketTCP(int32 m_Socket, const char* pAddress, uint16 port);

    private:
		std::string m_Address;
		uint16 m_Port;
    };
}

#endif
