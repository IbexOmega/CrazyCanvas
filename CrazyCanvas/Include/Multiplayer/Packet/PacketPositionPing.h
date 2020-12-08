#pragma once

#include "ECS/Entity.h"
#include "Multiplayer/Packet/Packet.h"

#include "Math/Math.h"

#pragma pack(push, 1)
struct PacketPositionPing
{
	DECL_PACKET(PacketPositionPing);
	glm::vec3 Position;
	uint64 PlayerUID;
};
#pragma pack(pop)
