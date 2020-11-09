#pragma once

#include "Multiplayer/Packet/Packet.h"

#pragma pack(push, 1)
struct PacketMatchStart : Packet
{
	DECL_PACKET(PacketMatchStart);
};
#pragma pack(pop)