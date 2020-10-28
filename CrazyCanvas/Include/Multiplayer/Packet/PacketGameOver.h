#pragma once

#include "Multiplayer/Packet/Packet.h"

#pragma pack(push, 1)
struct PacketGameOver
{
	DECL_PACKET(PacketGameOver);

	uint8 WinningTeamIndex = 0;
};
#pragma pack(pop)