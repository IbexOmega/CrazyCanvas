#pragma once

#include "LambdaEngine.h"

#include "Networking/API/IPEndPoint.h"

namespace LambdaEngine
{
	class NetworkSegment;
	class IPacketListener;
	class NetworkStatistics;

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
		virtual bool SendUnreliable(NetworkSegment* packet) = 0;
		virtual bool SendReliable(NetworkSegment* packet, IPacketListener* listener = nullptr) = 0;
		virtual const IPEndPoint& GetEndPoint() const = 0;
		virtual NetworkSegment* GetFreePacket(uint16 packetType) = 0;
		virtual EClientState GetState() const = 0;
		virtual const NetworkStatistics* GetStatistics() const = 0;
	};
}