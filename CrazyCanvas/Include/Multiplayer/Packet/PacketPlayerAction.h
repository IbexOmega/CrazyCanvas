#pragma once
#include "Multiplayer/Packet/Packet.h"

#include "Math/Math.h"

#include "ECS/Components/Player/ProjectileComponent.h"

#pragma pack(push, 1)
struct PacketPlayerAction : Packet
{
	DECL_PACKET(PacketPlayerAction);

	glm::quat	Rotation;
	bool		Walking			: 1 = true;
	bool		HoldingFlag		: 1 = false;
	bool		StartedReload	: 1 = false;
	int8		DeltaActionX	: 2 = 0;
	uint8		DeltaActionY	: 1 = 0;
	int8		DeltaActionZ	: 2 = 0;
	EAmmoType	FiredAmmo		= EAmmoType::AMMO_TYPE_NONE; // Default is that we fired no projectiles
	uint32		Angle			= 0;
};
#pragma pack(pop)