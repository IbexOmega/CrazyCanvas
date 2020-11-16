#pragma once

#include "Multiplayer/Packet/Packet.h"

#pragma pack(push, 1)
struct PacketPlayerPing
{
	DECL_PACKET(PacketPlayerPing);

	uint64 UID;
	uint16 Ping;
};
#pragma pack(pop)