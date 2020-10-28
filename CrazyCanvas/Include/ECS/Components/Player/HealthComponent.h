#pragma once
#include "ECS/Component.h"
#include "ECS/Components/Multiplayer/PacketComponent.h"

#include "Multiplayer/Packet/Packet.h"

#define START_HEALTH 100

/*
* HealthComponent
*/

struct HealthComponent
{
	DECL_COMPONENT(HealthComponent);
	int32 CurrentHealth = START_HEALTH;
};

/*
* HealthChangedPacket
*/

#pragma pack(push, 1)
struct HealthChangedPacket : Packet
{
	DECL_PACKET(HealthChangedPacket);
	int32 CurrentHealth = START_HEALTH;
};
#pragma pack(pop)