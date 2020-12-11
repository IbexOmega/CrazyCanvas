#pragma once

#include "ECS/Component.h"

#define GRENADE_COOLDOWN 5.0f					// Cooldown between throwing grenades. Specified in seconds.
#define GRENADE_FUSE_TIME 4.0f					// Time between a grenade being thrown and exploding. Specified in seconds.
#define GRENADE_INITIAL_SPEED 13.0f				// Speed in m/s at which a grenade is thrown
#define GRENADE_PLAYER_BLAST_RADIUS 4.0f		// In meters
#define GRENADE_ENVIRONMENT_BLAST_RADIUS 5.0f	// In meters

#define GRENADE_STATIC_FRICTION 1.4f
#define GRENADE_DYNAMIC_FRICTION 1.4f
#define GRENADE_RESTITUTION 0.18f
#define GRENADE_MASS 0.20f				// In kilograms

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
