#pragma once
#include "ECS/Component.h"
#include "ECS/Components/Multiplayer/PacketComponent.h"

/*
* HealthComponent
*/

struct HealthComponent
{
	DECL_COMPONENT(HealthComponent);
	int32 CurrentHealth = 100;
};

#pragma pack(push, 1)
struct PlayerHealthChangedPacket : Packet
{
	int32 PlayerNetworkUID	= -1;
	int32 NewHealth			 = 0;
};
#pragma pack(pop)