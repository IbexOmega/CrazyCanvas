#pragma once

#include "Application/API/Events/Event.h"
#include "ECS/Entity.h"
#include "Game/ECS/Systems/Physics/PhysicsSystem.h"

struct ProjectileHitEvent : LambdaEngine::Event
{
public:
	DECLARE_EVENT_TYPE(ProjectileHitEvent);

	inline ProjectileHitEvent(LambdaEngine::Entity entity, const LambdaEngine::CollisionInfo& collisionInfo)
		: Event()
		, Entity(entity)
		, CollisionInfo(collisionInfo)
	{
	}

	LambdaEngine::String ToString() const override final
	{
		return "ProjectileHitEvent={ Entity: " + std::to_string(Entity) + " }";
	}

public:
	LambdaEngine::Entity Entity;
	const LambdaEngine::CollisionInfo& CollisionInfo;
};
