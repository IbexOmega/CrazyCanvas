#pragma once

#include "Network/API/IClientTCPHandler.h"

class ClientTCPHandler : public LambdaEngine::IClientTCPHandler
{
public:
	ClientTCPHandler();
	~ClientTCPHandler();

	virtual void OnClientConnectedTCP(LambdaEngine::ClientTCP* client) override;
	virtual void OnClientDisconnectedTCP(LambdaEngine::ClientTCP* client) override;
	virtual void OnClientFailedConnectingTCP(LambdaEngine::ClientTCP* client) override;
	virtual void OnClientPacketReceivedTCP(LambdaEngine::ClientTCP* client, LambdaEngine::NetworkPacket* packet) override;
};
