#pragma once

#include "Multiplayer/Packet/Packet.h"

#pragma pack(push, 1)
struct PacketPlayerScore
{
	DECL_PACKET(PacketPlayerScore);

	uint64 UID;
	uint8 Team;
	uint8 Kills;
	uint8 Deaths;
	uint8 FlagsCaptured;
	uint8 FlagsDefended;
};
#pragma pack(pop)