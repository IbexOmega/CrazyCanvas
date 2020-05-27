#pragma once

#include "LambdaEngine.h"

#include "Networking/API/IClient.h"

namespace LambdaEngine
{
	class PacketManager;

	class LAMBDA_API IClientUDP : public IClient
	{
		friend class NetworkDebugger;

	public:
		DECL_INTERFACE(IClientUDP);

	protected:
		virtual PacketManager* GetPacketManager() = 0;
	};
}