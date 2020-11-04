#pragma once
#include "Multiplayer/Packet/Packet.h"

#include "Math/Math.h"

#pragma pack(push, 1)
struct PacketProjectileHit
{
	DECL_PACKET(PacketProjectileHit);
	uint8 Info = 0; // Bit 0-3: Team, Bit 4-6: Paint Mode, Bit 7: Was server
	glm::vec3 Position;
	glm::vec3 Direction;
};
#pragma pack(pop)