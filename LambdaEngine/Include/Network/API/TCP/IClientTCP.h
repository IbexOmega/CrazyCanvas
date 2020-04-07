#pragma once
#include "../IClient.h"

namespace LambdaEngine
{
	class LAMBDA_API IClientTCP : public IClient
	{
	public:
		DECL_INTERFACE(IClientTCP);

		/*
		* Connects the client to a given ip-address and port. To connect to a special address use
		* ADDRESS_LOOPBACK, ADDRESS_ANY, or ADDRESS_BROADCAST.
		*
		* address - The inet address to bind the socket to.
		* port    - The port to communicate through.
		*
		* return  - False if an error occured, otherwise true.
		*/
		virtual bool Connect(const std::string& address, uint16 port) = 0;

		/*
		* Disconnects the client
		*/
		virtual void Disconnect() = 0;

		/*
		* return - true if there is a valid connection, otherwise false
		*/
		virtual bool IsConnected() const = 0;

		/*
		* return - true if there is a valid connection, otherwise false
		*/
		virtual bool IsReadyToConnect() const = 0;
	};
}
