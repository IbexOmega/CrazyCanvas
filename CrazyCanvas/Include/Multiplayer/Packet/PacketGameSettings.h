#pragma once

#include "Multiplayer/Packet/Packet.h"

#include "Multiplayer/Packet/PacketJoin.h"

#pragma pack(push, 1)
struct PacketGameSettings
{
	DECL_PACKET(PacketGameSettings);
	
	char ServerName[MAX_NAME_LENGTH];
	uint8 Players			= 8;
	uint8 MapID				= 0;
	uint16 MaxTime			= 60 * 5;
	uint8 FlagsToWin		= 5;
	bool Visible			= false;
	bool ChangeTeam			= false;
	uint8 TeamColor0		= 0;
	uint8 TeamColor1		= 1;
};
#pragma pack(pop)