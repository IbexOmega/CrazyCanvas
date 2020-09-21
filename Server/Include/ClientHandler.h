#pragma once

#include "LambdaEngine.h"

#include "Networking/API/IClientRemoteHandler.h"
#include "Networking/API/IPacketListener.h"
#include "Time/API/Timestamp.h"

namespace LambdaEngine
{
	class NetworkSegment;
	class IClient;
}

class ClientHandler : public LambdaEngine::IClientRemoteHandler, public LambdaEngine::IPacketListener
{
public:
	ClientHandler();
	~ClientHandler();

	virtual void OnConnecting(LambdaEngine::IClient* pClient) override;
	virtual void OnConnected(LambdaEngine::IClient* pClient) override;
	virtual void OnDisconnecting(LambdaEngine::IClient* pClient) override;
	virtual void OnDisconnected(LambdaEngine::IClient* pClient) override;
	virtual void OnPacketReceived(LambdaEngine::IClient* pClient, LambdaEngine::NetworkSegment* pPacket) override;
	virtual void OnClientReleased(LambdaEngine::IClient* pClient) override;

	virtual void OnPacketDelivered(LambdaEngine::NetworkSegment* pPacket) override;
	virtual void OnPacketResent(LambdaEngine::NetworkSegment* pPacket, uint8 retries) override;
	virtual void OnPacketMaxTriesReached(LambdaEngine::NetworkSegment* pPacket, uint8 retries) override;

private:
	LambdaEngine::Timestamp m_BenchMarkTimer;
	int m_Counter = 0;
};