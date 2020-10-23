#pragma once
#include "ECS/Component.h"

/*
* HealthComponent
*/

struct HealthComponent
{
	DECL_COMPONENT(HealthComponent);
	int32 CurrentHealth = 100;
};