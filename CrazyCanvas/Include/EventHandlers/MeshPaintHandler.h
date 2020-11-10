#pragma once
#include "Physics/PhysicsEvents.h"

#include "Rendering/PaintMaskRenderer.h"
#include "Multiplayer/Packet/MultiplayerEvents.h"
#include "Multiplayer/Packet/PacketProjectileHit.h"

#include <queue>

/*
* Helpers
*/

#define SET_TEAM_INDEX(mask, value) \
	mask |= (((uint8)value) & 0x0F)

#define SET_PAINT_MODE(mask, value) \
	mask |= ((((uint8)value) & 0x0F) << 4)

#define GET_TEAM_INDEX(mask) (ETeam)(mask & 0x0F)
#define GET_PAINT_MODE(mask) (EPaintMode)((mask & 0xF0) >> 4)

#define IS_MASK_PAINTED(mask)			(bool)(mask & 0x01)
#define GET_TEAM_INDEX_FROM_MASK(mask)	(ETeam)((mask >> 1) & 0x03)

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
	};

private:
	bool OnProjectileHit(const ProjectileHitEvent& projectileHitEvent);

	bool OnPacketProjectileHitReceived(const PacketReceivedEvent<PacketProjectileHit>& event);

	bool IsPaintPointEqual(PaintPoint& a, PaintPoint& b);

private:
	std::queue<PaintPoint> m_PaintPointsOnClient;

};
