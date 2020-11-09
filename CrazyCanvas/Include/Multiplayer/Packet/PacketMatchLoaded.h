#pragma once

#include "Multiplayer/Packet/Packet.h"

#pragma pack(push, 1)
struct PacketMatchLoaded
{
	DECL_PACKET(PacketMatchLoaded);
};
#pragma pack(pop)