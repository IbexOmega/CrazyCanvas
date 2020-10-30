#pragma once

#include "Multiplayer/Packet/Packet.h"
#include "ECS/Components/Match/FlagComponent.h"

#pragma pack(push, 1)
struct PacketFlagEdited : Packet
{
	DECL_PACKET(PacketFlagEdited);

	EFlagPacketType	FlagPacketType;
	int32			PickedUpNetworkUID = -1;
	glm::vec3		DroppedPosition;
};
#pragma pack(pop)