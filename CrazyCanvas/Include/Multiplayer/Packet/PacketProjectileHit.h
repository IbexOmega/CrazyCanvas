#pragma once
#include "Multiplayer/Packet/Packet.h"

#include "Math/Math.h"

#pragma pack(push, 1)
struct PacketProjectileHit
{
	DECL_PACKET(PacketProjectileHit);
	uint8 Info = 0; // Bit 0-3: Team, Bit 4-7: Paint Mode
	glm::vec3 Position;
	glm::vec3 Direction;
	uint32 Angle = 0;
};
#pragma pack(pop)