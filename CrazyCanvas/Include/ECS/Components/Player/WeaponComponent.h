#pragma once

#include "ECS/Component.h"
#include "ECS/Entity.h"

struct WeaponComponent
{
	DECL_COMPONENT(WeaponComponent);
	// The entity holding the weapon is separate from the weapon entity itself. This needs to be set.
	LambdaEngine::Entity WeaponOwner = UINT32_MAX;
	float32	FireRate			= 4.0f; // Times per second the weapon can be fired
	float32 ReloadTime			= 5.0f;
	float32	CurrentCooldown		= 0.0f; // Time until the weapon can be fired again
	float32 ReloadClock			= 0.0f;
	int32	CurrentAmmunition	= 5;
};
