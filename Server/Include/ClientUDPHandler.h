#pragma once

#include "Network/API/UDP/IClientUDPHandler.h"

class ClientUDPHandler : public LambdaEngine::IClientUDPHandler
{
public:
	ClientUDPHandler();
	~ClientUDPHandler();

	virtual void OnClientPacketReceivedUDP(LambdaEngine::IClientUDP* client, LambdaEngine::NetworkPacket* packet) override;
	virtual void OnClientErrorUDP(LambdaEngine::IClientUDP* client) override;
	virtual void OnClientStoppedUDP(LambdaEngine::IClientUDP* client) override;
};
