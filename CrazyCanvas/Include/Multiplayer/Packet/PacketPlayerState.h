#pragma once

#include "Multiplayer/Packet/Packet.h"

#include "Lobby/Player.h"

#pragma pack(push, 1)
struct PacketPlayerState
{
	DECL_PACKET(PacketPlayerState);

	uint64 UID;
	EGameState State;
};
#pragma pack(pop)