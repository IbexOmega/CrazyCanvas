#pragma once
#include "Physics/PhysicsEvents.h"

#include "Multiplayer/Packet/MultiplayerEvents.h"
#include "Multiplayer/Packet/PacketProjectileHit.h"

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
