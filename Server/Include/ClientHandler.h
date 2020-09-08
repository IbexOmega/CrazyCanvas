#pragma once

#include "LambdaEngine.h"

#include "Networking/API/IClientRemoteHandler.h"

namespace LambdaEngine
{
	class NetworkSegment;
	class IClient;
}

class ClientHandler : public LambdaEngine::IClientRemoteHandler
{
public:
	ClientHandler();
	~ClientHandler();

	virtual void OnConnecting(LambdaEngine::IClient* pClient) override;
	virtual void OnConnected(LambdaEngine::IClient* pClient) override;
	virtual void OnDisconnecting(LambdaEngine::IClient* pClient) override;
	virtual void OnDisconnected(LambdaEngine::IClient* pClient) override;
	virtual void OnPacketReceived(LambdaEngine::IClient* pClient, LambdaEngine::NetworkSegment* pPacket) override;

private:
	int counter = 0;
};