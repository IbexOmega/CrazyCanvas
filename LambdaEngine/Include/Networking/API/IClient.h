#pragma once

#include "LambdaEngine.h"

#include "Networking/API/IPEndPoint.h"

namespace LambdaEngine
{
	class NetworkPacket;
	class IPacketListener;

	enum EClientState
	{
		STATE_DISCONNECTED,
		STATE_DISCONNECTING,
		STATE_CONNECTING,
		STATE_CONNECTED
	};

	class LAMBDA_API IClient
	{
	public:

	public:
		DECL_INTERFACE(IClient);

		virtual void Disconnect() = 0;
		virtual void Release() = 0;
		virtual bool IsConnected() = 0;
		virtual bool SendUnreliable(NetworkPacket* packet) = 0;
		virtual bool SendReliable(NetworkPacket* packet, IPacketListener* listener) = 0;
		virtual const IPEndPoint& GetEndPoint() const = 0;
		virtual NetworkPacket* GetFreePacket(uint16 packetType) = 0;
		virtual EClientState GetState() const = 0;
	};
}