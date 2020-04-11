#pragma once

#include "Defines.h"

namespace LambdaEngine
{
	class RemoteGameClient;
	class NetworkPacket;

	class LAMBDA_API IGameClientHandler
	{
	public:
		DECL_INTERFACE(IGameClientHandler);

		virtual void OnPacketReceived(RemoteGameClient* client, NetworkPacket* packet) = 0;
	};
}
