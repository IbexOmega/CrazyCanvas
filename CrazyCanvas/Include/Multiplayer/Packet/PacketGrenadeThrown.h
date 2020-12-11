#pragma once
#include "Multiplayer/Packet/Packet.h"

#include "Math/Math.h"

#pragma pack(push, 1)
struct PacketGrenadeThrown : Packet
{
	DECL_PACKET(PacketGrenadeThrown);
	glm::vec3 Position;
	glm::vec3 Velocity;
	uint64 PlayerUID;
};
#pragma pack(pop)