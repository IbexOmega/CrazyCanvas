#pragma once

#include "Multiplayer/Packet/Packet.h"

#pragma pack(push, 1)
struct PacketPlayerAliveChanged
{
	DECL_PACKET(PacketPlayerAliveChanged);

	uint64 UID;
	bool IsDead;
	uint8 Deaths;
};
#pragma pack(pop)