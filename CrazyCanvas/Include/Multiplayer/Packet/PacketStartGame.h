#pragma once

#include "Multiplayer/Packet/Packet.h"

//A packet sent from the Client who is the host, to tell the server that Match should load

#pragma pack(push, 1)
struct PacketStartGame
{
	DECL_PACKET(PacketStartGame);
	int32 AuthenticationID = -1;
};
#pragma pack(pop)