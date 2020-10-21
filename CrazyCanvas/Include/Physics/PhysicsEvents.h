#pragma once
#include "Application/API/Events/Event.h"

#include "ECS/Entity.h"
#include "ECS/Components/Player/ProjectileComponent.h"

#include "Game/ECS/Systems/Physics/PhysicsSystem.h"

struct ProjectileHitEvent : LambdaEngine::Event
{
public:
	DECLARE_EVENT_TYPE(ProjectileHitEvent);

	inline ProjectileHitEvent(const LambdaEngine::EntityCollisionInfo& collisionInfo0, const LambdaEngine::EntityCollisionInfo& collisionInfo1, const EAmmoType ammoType)
		: Event()
		, CollisionInfo0(collisionInfo0)
		, CollisionInfo1(collisionInfo1)
		, AmmoType(ammoType)
	{
	}

	LambdaEngine::String ToString() const override final
	{
		return "ProjectileHitEvent={ Entity: " + std::to_string(Entity) + " }";
	}

public:
	LambdaEngine::Entity Entity; // Is this used?
	const LambdaEngine::EntityCollisionInfo& CollisionInfo0;
	const LambdaEngine::EntityCollisionInfo& CollisionInfo1;
	const EAmmoType AmmoType;
};
