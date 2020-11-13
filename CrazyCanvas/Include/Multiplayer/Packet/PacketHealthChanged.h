#pragma once
#include "Multiplayer/Packet/Packet.h"

#include "ECS/Components/Player/HealthComponent.h"

#pragma pack(push, 1)
struct PacketHealthChanged : Packet
{
	DECL_PACKET(PacketHealthChanged);
	int32	CurrentHealth	= START_HEALTH;
};
#pragma pack(pop)