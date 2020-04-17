#pragma once

#include "LambdaEngine.h"

#include "Networking/API/IClientUDPHandler.h"

namespace LambdaEngine
{
	class NetworkPacket;
	class IClientUDP;
}

class ClientUDPHandler : public LambdaEngine::IClientUDPHandler
{
public:
	ClientUDPHandler();
	~ClientUDPHandler();

	virtual void OnPacketUDPReceived(LambdaEngine::IClientUDP* pClient, LambdaEngine::NetworkPacket* pPacket) override;
};