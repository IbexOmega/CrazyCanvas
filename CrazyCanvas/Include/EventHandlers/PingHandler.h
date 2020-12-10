#pragma once

#include "Containers/IDVector.h"
#include "ECS/EntitySubscriber.h"
#include "Events/PacketEvents.h"
#include "Multiplayer/Packet/PacketPositionPing.h"

#define MAX_PING_DISTANCE 100.0f
#define PING_DURATION 8.0f // Seconds until a ping is removed (unless it is replaced with a new ping)

namespace LambdaEngine
{
	class ISoundEffect3D;
	struct KeyPressedEvent;
}

class PingHandler : LambdaEngine::EntitySubscriber
{
public:
	PingHandler() = default;
	~PingHandler();

	void Init();

private:
	void Ping(LambdaEngine::Entity pingingPlayer, const glm::vec3& pingPosition, const glm::vec3& cameraPosition, uint8 teamIndex);

	bool OnKeyPress(const LambdaEngine::KeyPressedEvent& keyPressEvent);
	bool OnPacketPositionPingReceived(const PacketReceivedEvent<PacketPositionPing>& pingEvent);

private:
	// Map player entities (not camera entities) to their current ping entities
	LambdaEngine::IDDVector<LambdaEngine::Entity> m_PlayerPings;
	LambdaEngine::IDVector m_LocalPlayerCamera;

	LambdaEngine::ISoundEffect3D* m_pPingSound = nullptr;
};
