#pragma once

#include "Multiplayer/Packet/Packet.h"

#pragma pack(push, 1)
struct PacketConfigureServer
{
	DECL_PACKET(PacketConfigureServer);

	int32 AuthenticationID = -1;
	uint8 Players = 0;
	uint8 MapID = 0;
};
#pragma pack(pop)