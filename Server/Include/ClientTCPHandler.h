#pragma once

#include "Network/API/IClientTCPHandler.h"

class ClientTCPHandler : public LambdaEngine::IClientTCPHandler
{
public:
	ClientTCPHandler();
	~ClientTCPHandler();

	virtual void OnClientConnected(LambdaEngine::ClientTCP2* client) override;
	virtual void OnClientDisconnected(LambdaEngine::ClientTCP2* client) override;
	virtual void OnClientFailedConnecting(LambdaEngine::ClientTCP2* client) override;
	virtual void OnClientPacketReceived(LambdaEngine::ClientTCP2* client, LambdaEngine::NetworkPacket* packet) override;
};
