#pragma once

#include "Multiplayer/Packet/Packet.h"

#pragma pack(push, 1)
struct PacketPlayerDied
{
	DECL_PACKET(PacketPlayerDied);

	uint64 UID;
};
#pragma pack(pop)