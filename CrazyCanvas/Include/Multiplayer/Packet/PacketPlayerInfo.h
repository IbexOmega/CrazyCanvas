#pragma once

#include "Multiplayer/Packet/Packet.h"

#include "Lobby/Player.h"

#pragma pack(push, 1)
struct PacketPlayerInfo
{
	DECL_PACKET(PacketPlayerInfo);

	uint64 UID;
	bool IsHost;
	uint16 Ping;
	EPlayerState State;
	uint8 Team;
	uint8 Kills;
	uint8 Deaths;
	uint8 FlagsCaptured;
	uint8 FlagsDefended;
};
#pragma pack(pop)