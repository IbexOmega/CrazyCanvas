#pragma once

#include "Network/API/IClientTCPHandler.h"

class ClientTCPHandler : public LambdaEngine::IClientTCPHandler
{
public:
	ClientTCPHandler();
	~ClientTCPHandler();

	virtual void OnClientConnected(LambdaEngine::ClientTCP* client) override;
	virtual void OnClientDisconnected(LambdaEngine::ClientTCP* client) override;
	virtual void OnClientFailedConnecting(LambdaEngine::ClientTCP* client) override;
	virtual void OnClientPacketReceived(LambdaEngine::ClientTCP* client, LambdaEngine::NetworkPacket* packet) override;
};
