#pragma once

#include "LambdaEngine.h"

#include "Networking/API/IPEndPoint.h"

namespace LambdaEngine
{
	class NetworkPacket;
	class IPacketListener;

	class LAMBDA_API IClient
	{
	public:
		DECL_INTERFACE(IClient);

		virtual bool Connect(const IPEndPoint& ipEndPoint) = 0;
		virtual void Disconnect() = 0;
		virtual bool IsConnected() = 0;
		virtual bool SendUnreliable(NetworkPacket* packet) = 0;
		virtual bool SendReliable(NetworkPacket* packet, IPacketListener* listener) = 0;
		virtual const IPEndPoint& GetEndPoint() const = 0;
		virtual NetworkPacket* GetFreePacket() = 0;
	};
}