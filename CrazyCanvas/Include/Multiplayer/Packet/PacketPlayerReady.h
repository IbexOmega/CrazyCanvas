#pragma once

#include "Multiplayer/Packet/Packet.h"

#include "Lobby/Player.h"

#pragma pack(push, 1)
struct PacketPlayerReady
{
	DECL_PACKET(PacketPlayerReady);

	uint64 UID;
	bool IsReady;
};
#pragma pack(pop)