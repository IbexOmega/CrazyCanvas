#pragma once

#include "Application/API/Events/Event.h"
#include "ECS/Entity.h"
#include "Game/ECS/Systems/Physics/PhysicsSystem.h"

struct ProjectileHitEvent : LambdaEngine::Event
{
public:
	DECLARE_EVENT_TYPE(ProjectileHitEvent);

	inline ProjectileHitEvent(const LambdaEngine::EntityCollisionInfo& collisionInfo0, const LambdaEngine::EntityCollisionInfo& collisionInfo1)
		: Event()
		, CollisionInfo0(collisionInfo0)
		, CollisionInfo1(collisionInfo1)
	{}

	LambdaEngine::String ToString() const override final
	{
		return "ProjectileHitEvent={ Entity: " + std::to_string(Entity) + " }";
	}

public:
	LambdaEngine::Entity Entity;
	const LambdaEngine::EntityCollisionInfo& CollisionInfo0, &CollisionInfo1;
};
