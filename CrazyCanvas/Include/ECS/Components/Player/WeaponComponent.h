#pragma once

#include "ECS/Component.h"
#include "ECS/Entity.h"

#include "ProjectileComponent.h"
#include <unordered_map>
#include <utility>

#define AMMO_CAPACITY 50

/*
* WeaponComponent
*/
struct WeaponComponent
{
	DECL_COMPONENT(WeaponComponent);
	// The entity holding the weapon is separate from the weapon entity itself. This needs to be set.
	LambdaEngine::Entity WeaponOwner = UINT32_MAX;
	float32	FireRate			= 4.0f; // Times per second the weapon can be fired
	float32 ReloadTime			= 2.0f;
	float32	CurrentCooldown		= 0.0f; // Time until the weapon can be fired again
	float32 ReloadClock			= 0.0f;

	std::unordered_map<EAmmoType, std::pair<int32, int32>> WeaponTypeAmmo =
		{
			{ EAmmoType::AMMO_TYPE_PAINT, {AMMO_CAPACITY, AMMO_CAPACITY} },
			{ EAmmoType::AMMO_TYPE_WATER, {AMMO_CAPACITY, AMMO_CAPACITY} }
		};

};

/*
* WeaponLocalComponent
*/
struct WeaponLocalComponent
{
	DECL_COMPONENT(WeaponLocalComponent);
};