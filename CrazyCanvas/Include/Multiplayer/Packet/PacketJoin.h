#pragma once

#include "Multiplayer/Packet/Packet.h"

#define MAX_PLAYER_NAME_LENGTH 16

#pragma pack(push, 1)
struct PacketJoin
{
	DECL_PACKET(PacketJoin);

	uint64 UID = 0;
	char Name[MAX_PLAYER_NAME_LENGTH];
};
#pragma pack(pop)