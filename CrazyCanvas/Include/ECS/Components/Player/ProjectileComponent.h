#pragma once
#include "ECS/Component.h"

/*
* EAmmoType
*/

enum class EAmmoType : uint8
{
	AMMO_TYPE_NONE	= 0,
	AMMO_TYPE_PAINT	= 1,
	AMMO_TYPE_WATER	= 2
};

/*
* ProjectileComponent
*/

struct ProjectileComponent
{
	DECL_COMPONENT(ProjectileComponent);
	EAmmoType AmmoType;
	uint32 Angle = 0;
	LambdaEngine::Entity Owner;
};