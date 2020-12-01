#pragma once

#include "Multiplayer/Packet/Packet.h"

#include "Multiplayer/Packet/PacketJoin.h"
#include "Match/MatchGameMode.h"

#pragma pack(push, 1)
struct PacketGameSettings
{
	DECL_PACKET(PacketGameSettings);
	
	char ServerName[MAX_NAME_LENGTH];
	uint8 Players			= 10;
	uint8 MapID				= 0;
	uint16 MaxTime			= 60 * 5;
	uint8 FlagsToWin		= 5;
	bool Visible			= false;
	bool ChangeTeam			= false;
	EGameMode GameMode		= EGameMode::CTF_TEAM_FLAG;
	uint8 TeamColor1		= 1;
	uint8 TeamColor2		= 2;
};
#pragma pack(pop)