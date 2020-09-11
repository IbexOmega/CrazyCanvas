#pragma once

#include "LambdaEngine.h"

namespace LambdaEngine
{
	class NetworkSegment;

	class LAMBDA_API IPacketListener
	{
	public:
		DECL_INTERFACE(IPacketListener);

		virtual void OnPacketDelivered(NetworkSegment* pPacket) = 0;
		virtual void OnPacketResent(NetworkSegment* pPacket, uint8 retries) = 0;
		virtual void OnPacketMaxTriesReached(NetworkSegment* pPacket, uint8 retries) = 0;
	};
}