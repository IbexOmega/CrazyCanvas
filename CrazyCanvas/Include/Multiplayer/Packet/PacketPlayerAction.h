#pragma once
#include "Multiplayer/Packet/Packet.h"

#include "Math/Math.h"

#include "ECS/Components/Player/ProjectileComponent.h"

#pragma pack(push, 1)
struct PacketPlayerAction : Packet
{
	DECL_PACKET(PacketPlayerAction);

	glm::quat Rotation;
	glm::i8vec3 DeltaAction;
	bool Walking;
	bool StartedReload	= false;
	EAmmoType FiredAmmo	= EAmmoType::AMMO_TYPE_NONE; // Default is that we fired no projectiles
	uint32 Angle		= 0;
};
#pragma pack(pop)