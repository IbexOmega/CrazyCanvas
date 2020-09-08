#pragma once

#include "LambdaEngine.h"

namespace LambdaEngine
{
	class NetworkSegment;
	class IClient;

	class LAMBDA_API IClientRemoteHandler
	{
	public:
		DECL_INTERFACE(IClientRemoteHandler);

		virtual void OnConnecting(IClient* pClient) = 0;
		virtual void OnConnected(IClient* pClient) = 0;
		virtual void OnDisconnecting(IClient* pClient) = 0;
		virtual void OnDisconnected(IClient* pClient) = 0;
		virtual void OnPacketReceived(IClient* pClient, NetworkSegment* pPacket) = 0;
	};
}