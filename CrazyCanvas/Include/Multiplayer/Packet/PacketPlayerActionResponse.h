#pragma once
#include "Multiplayer/Packet/Packet.h"

#include "Math/Math.h"

#include "ECS/Components/Player/ProjectileComponent.h"

#pragma pack(push, 1)
struct PacketPlayerActionResponse : Packet
{
	DECL_PACKET(PacketPlayerActionResponse);

	glm::vec3 Position;
	glm::vec3 Velocity;
	glm::quat Rotation;

	EAmmoType FiredAmmo = EAmmoType::AMMO_TYPE_NONE; // Default is that we fired no projectiles
	glm::vec3 WeaponPosition;
	glm::vec3 WeaponVelocity;
};
#pragma pack(pop)