#pragma once

#include "LambdaEngine.h"

namespace LambdaEngine
{
	class NetworkPacket;

	class LAMBDA_API IPacketListener
	{
	public:
		DECL_INTERFACE(IPacketListener);

		virtual void OnPacketDelivered(NetworkPacket* pPacket) = 0;
		virtual void OnPacketResent(NetworkPacket* pPacket, uint8 retries) = 0;
		virtual void OnPacketMaxTriesReached(NetworkPacket* pPacket, uint8 retries) = 0;
	};
}