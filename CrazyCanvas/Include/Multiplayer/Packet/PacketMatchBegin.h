#pragma once

#include "Multiplayer/Packet/Packet.h"

#pragma pack(push, 1)
struct PacketMatchBegin
{
	DECL_PACKET(PacketMatchBegin);
};
#pragma pack(pop)