#pragma once

#include "Multiplayer/Packet/Packet.h"

#pragma pack(push, 1)
struct PacketMatchLoad
{
	DECL_PACKET(PacketMatchLoad);

	uint8 MapID;
};
#pragma pack(pop)