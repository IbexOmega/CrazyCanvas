#pragma once

#include "Multiplayer/Packet/Packet.h"

#pragma pack(push, 1)
struct PacketGameSettings
{
	DECL_PACKET(PacketGameSettings);

	uint8 Players			= 8;
	uint8 MapID				= 0;
	uint16 MaxTime			= 60 * 5;
	uint8 FlagsToWin		= 5;
	bool Visible			= false;
	bool ChangeTeam			= false;
};
#pragma pack(pop)