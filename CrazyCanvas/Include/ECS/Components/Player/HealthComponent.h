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