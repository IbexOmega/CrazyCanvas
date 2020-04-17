#pragma once

#include "LambdaEngine.h"

namespace LambdaEngine
{
	class NetworkPacket;
	class IClientUDP;

	class LAMBDA_API IClientUDPHandler
	{
	public:
		DECL_INTERFACE(IClientUDPHandler);

		virtual void OnPacketUDPReceived(IClientUDP* pClient, NetworkPacket* pPacket) = 0;
	};
}