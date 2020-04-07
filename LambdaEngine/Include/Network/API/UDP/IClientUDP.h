#pragma once
#include "../IClient.h"

namespace LambdaEngine
{
	class LAMBDA_API IClientUDP : public IClient
	{
	public:
		DECL_INTERFACE(IClientUDP);

		/*
		* Start the client and set the given ip-address and port. To send to a special address use
		* ADDRESS_LOOPBACK, ADDRESS_ANY, or ADDRESS_BROADCAST.
		*
		* address - The inet address to bind the socket to.
		* port    - The port to communicate through.
		*
		* return  - False if an error occured, otherwise true.
		*/
		virtual bool Start(const std::string& address, uint16 port) = 0;
	};
}
