#pragma once

#include "Multiplayer/Packet/Packet.h"

#pragma pack(push, 1)
struct PacketDeleteLevelObject
{
	DECL_PACKET(PacketDeleteLevelObject);
	int32 NetworkUID = -1;
};
#pragma pack(pop)