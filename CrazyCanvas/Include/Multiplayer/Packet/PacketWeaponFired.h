#pragma once

#include "Multiplayer/Packet/Packet.h"

#pragma pack(push, 1)
struct PacketWeaponFired : Packet
{
	DECL_PACKET(PacketWeaponFired);

	glm::vec3	FirePosition;
	glm::vec3	InitalVelocity;
	glm::quat	FireDirection;
	EAmmoType	AmmoType;
};
#pragma pack(pop)