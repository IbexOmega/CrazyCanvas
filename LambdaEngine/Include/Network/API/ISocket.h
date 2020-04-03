#pragma once
#include "Defines.h"
#include "Types.h"

#include <string>

#define ADDRESS_LOOPBACK	"ADDRESS_LOOPBACK"
#define ADDRESS_ANY			"ADDRESS_ANY"
#define ADDRESS_BROADCAST	"ADDRESS_BROADCAST"

namespace LambdaEngine
{
	class ISocket
	{
	public:
		DECL_INTERFACE(ISocket);

		/*
		* Binds the socket to a given ip-address and port. To bind a special address use
		* ADDRESS_LOOPBACK, ADDRESS_ANY, or ADDRESS_BROADCAST.
		*
		* address - The inet address to bind the socket to.
		* port    - The port to communicate through.
		* return  - False if an error occured, otherwise true.
		*/
		virtual bool Bind(const std::string& address, uint16 port) = 0;

		/*
		* Connects the socket to a given ip-address and port. To bind a special address use
		* ADDRESS_LOOPBACK, ADDRESS_ANY, or ADDRESS_BROADCAST.
		*
		* address - The inet address to bind the socket to.
		* port    - The port to communicate through.
		* return  - False if an error occured, otherwise true.
		*/
		virtual bool Connect(const std::string& address, uint16 port) = 0;

		/*
		* Sets the socket in non blocking or blocking mode.
		*
		* return - False if an error occured, otherwise true.
		*/
		virtual bool SetNonBlocking(bool nonBlocking) = 0;
		virtual bool IsNonBlocking() const = 0;

		/*
		* Closes the socket
		*
		* return - False if an error occured, otherwise true.
		*/
		virtual bool Close() = 0;
		virtual bool IsClosed() const = 0;

		/*
		* return - The currently used inet address.
		*/
		virtual const std::string& GetAddress() const = 0;

		/*
		* return - The currently used port.
		*/
		virtual uint16 GetPort() const = 0;
	};
}
