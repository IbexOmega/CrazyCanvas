#pragma once

#include "LambdaEngine.h"

#include "Networking/API/IClientRemoteHandler.h"
#include "Networking/API/IPacketListener.h"

#include "Time/API/Timestamp.h"

#include "ECS/Entity.h"

#include "Math/Math.h"

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
	uint32 m_Counter = 0;
	glm::vec3 m_Position;
	glm::vec3 m_Color;
	LambdaEngine::Entity m_Entity;
};