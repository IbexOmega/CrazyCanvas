#pragma once
#include "Multiplayer/Packet/Packet.h"

#include "Math/Math.h"

#include "ECS/Components/Player/ProjectileComponent.h"

#pragma pack(push, 1)
struct PlayerAction : Packet
{
	DECL_PACKET(PlayerAction);

	glm::quat Rotation;
	int8 DeltaForward	= 0;
	int8 DeltaLeft		= 0;
	bool StartedReload	= false;
	EAmmoType FiredAmmo	= EAmmoType::AMMO_TYPE_NONE; // Default is that we fired no projectiles
};
#pragma pack(pop)