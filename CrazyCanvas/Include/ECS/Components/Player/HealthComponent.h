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