#pragma once
#include "Defines.h"

namespace LambdaEngine
{
	class IClientUDP;
	class NetworkPacket;
	class IClientUDPHandler;

	class LAMBDA_API IServerUDPHandler
	{
	public:
		DECL_INTERFACE(IServerUDPHandler);

		/*
		* Called to create a client handler for a ClientUDP object
		*
		* return  - a new handler
		*/
		virtual IClientUDPHandler* CreateClientHandlerUDP() = 0;
	};
}
