#pragma once
#include "Multiplayer/Packet/Packet.h"

#pragma pack(push, 1)
struct PacketResetPlayerTexture : Packet
{
	DECL_PACKET(PacketResetPlayerTexture);
};
#pragma pack(pop)