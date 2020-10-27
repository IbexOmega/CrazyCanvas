#pragma once
#include "ProjectileComponent.h"

#include "ECS/Component.h"
#include "ECS/Components/Multiplayer/PacketComponent.h"
#include "ECS/Entity.h"

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
	int32	CurrentAmmunition	= AMMO_CAPACITY;
	int32   AmmoCapacity		= AMMO_CAPACITY;
};

/*
* WeaponFiredPacket
*/

#pragma pack(push, 1)
struct WeaponFiredPacket : Packet
{
	glm::vec3	FirePosition;
	glm::vec3	InitalVelocity;
	glm::quat	FireDirection;
	EAmmoType	AmmoType;
};
#pragma pack(pop)