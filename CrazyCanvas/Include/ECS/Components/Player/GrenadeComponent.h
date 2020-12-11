#pragma once

#include "ECS/Component.h"

#define GRENADE_COOLDOWN 20.0f					// Cooldown between throwing grenades. Specified in seconds.
#define GRENADE_FUSE_TIME 4.0f					// Time between a grenade being thrown and exploding. Specified in seconds.
#define GRENADE_INITIAL_SPEED 5.0f				// Speed in m/s at which a grenade is thrown
#define GRENADE_PLAYER_BLAST_RADIUS 3.0f		// In meters
#define GRENADE_ENVIRONMENT_BLAST_RADIUS 6.0f	// In meters

struct GrenadeWielderComponent
{
	DECL_COMPONENT(GrenadeWielderComponent);
	float32 ThrowCooldown;
};

struct GrenadeComponent
{
	DECL_COMPONENT(GrenadeComponent);
	float32 ExplosionCountdown;
};
