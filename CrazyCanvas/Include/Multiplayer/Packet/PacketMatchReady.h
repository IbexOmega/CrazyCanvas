#pragma once

#include "Multiplayer/Packet/Packet.h"

#pragma pack(push, 1)
struct PacketMatchReady
{
	DECL_PACKET(PacketMatchReady);
};
#pragma pack(pop)