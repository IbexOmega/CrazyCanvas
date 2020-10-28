#pragma once

#include "Multiplayer/Packet/Packet.h"

#pragma pack(push, 1)
struct PacketHostServer
{
	DECL_PACKET(PacketHostServer);

	int8 PlayersNumber = -1;
	int8 MapNumber = -1;
	int32 AuthenticationID = -1;
};
#pragma pack(pop)