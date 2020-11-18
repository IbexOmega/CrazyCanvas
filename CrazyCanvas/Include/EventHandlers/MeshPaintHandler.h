#pragma once
#include "Physics/PhysicsEvents.h"

#include "Multiplayer/Packet/PacketPlayerAliveChanged.h"
#include "Multiplayer/Packet/PacketProjectileHit.h"

#include "Events/PlayerEvents.h"
#include "Events/PacketEvents.h"
#include "Events/GameplayEvents.h"

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
	bool OnProjectileHit(const ProjectileHitEvent& projectileHitEvent);
	bool OnPacketProjectileHitReceived(const PacketReceivedEvent<PacketProjectileHit>& event);
};
