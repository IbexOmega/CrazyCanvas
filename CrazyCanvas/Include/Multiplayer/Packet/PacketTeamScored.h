#pragma once

#include "Multiplayer/Packet/Packet.h"

#pragma pack(push, 1)
struct PacketTeamScored
{
	DECL_PACKET(PacketTeamScored);

	uint8 TeamIndex = 0;
	uint32 Score = 0;
	uint64 PlayerUID = 0;
};
#pragma pack(pop)