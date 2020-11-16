#pragma once

#include "Multiplayer/Packet/Packet.h"

#pragma pack(push, 1)
struct PacketPlayerHost
{
	DECL_PACKET(PacketPlayerHost);

	uint64 UID;
};
#pragma pack(pop)