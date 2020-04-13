#pragma once

#include "LambdaEngine.h"

namespace LambdaEngine
{
	class NetworkPacket;

	class LAMBDA_API IPacketListener
	{
	public:
		DECL_INTERFACE(IPacketListener);

		virtual void OnPacketDelivered(NetworkPacket* packet) = 0;
		virtual void OnPacketResent(NetworkPacket* packet) = 0;
	};
}