#pragma once
#include "Physics/PhysicsEvents.h"

#include "Rendering/PaintMaskRenderer.h"
#include "Multiplayer/Packet/MultiplayerEvents.h"
#include "Multiplayer/Packet/PacketProjectileHit.h"

#include <queue>

// MeshPaintHandler listens to events that should cause mesh to be painted
class MeshPaintHandler
{
public:
	MeshPaintHandler() = default;
	~MeshPaintHandler();

	void Init();

private:
	struct PaintPoint
	{
		glm::vec3 Position;
		glm::vec3 Direction;
		LambdaEngine::EPaintMode	PaintMode = LambdaEngine::EPaintMode::NONE;
		LambdaEngine::ERemoteMode	RemoteMode = LambdaEngine::ERemoteMode::UNDEFINED;
		LambdaEngine::ETeam			Team = LambdaEngine::ETeam::NONE;
		float Angle;
	};

private:
	bool OnProjectileHit(const ProjectileHitEvent& projectileHitEvent);

	bool OnPacketProjectileHitReceived(const PacketReceivedEvent<PacketProjectileHit>& event);

	bool IsPaintPointEqual(PaintPoint& a, PaintPoint& b);

private:
	std::queue<PaintPoint> m_PaintPointsOnClient;

};
