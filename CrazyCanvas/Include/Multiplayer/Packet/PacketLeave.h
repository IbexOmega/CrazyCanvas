#pragma once

#include "Multiplayer/Packet/Packet.h"

#pragma pack(push, 1)
struct PacketLeave
{
	DECL_PACKET(PacketLeave);

	uint64 UID = 0;
};
#pragma pack(pop)