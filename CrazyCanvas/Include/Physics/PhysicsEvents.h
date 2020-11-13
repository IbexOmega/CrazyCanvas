#pragma once
#include "Application/API/Events/Event.h"

#include "ECS/Entity.h"
#include "ECS/Components/Player/ProjectileComponent.h"

#include "Game/ECS/Systems/Physics/PhysicsSystem.h"

#include "Rendering/PaintMaskRenderer.h"

struct ProjectileHitEvent : LambdaEngine::Event
{
public:
	DECLARE_EVENT_TYPE(ProjectileHitEvent);

	inline ProjectileHitEvent(
			const LambdaEngine::EntityCollisionInfo& collisionInfo0, 
			const LambdaEngine::EntityCollisionInfo& collisionInfo1, 
			const EAmmoType ammoType,
			const LambdaEngine::ETeam team,
			const uint32 angle)
		: Event()
		, CollisionInfo0(collisionInfo0)
		, CollisionInfo1(collisionInfo1)
		, AmmoType(ammoType)
		, Team(team)
		, Angle(angle)
	{
	}

	LambdaEngine::String ToString() const override final
	{
		return "ProjectileHitEvent={ Entity: " + std::to_string(CollisionInfo1.Entity) + " }";
	}

public:
	const LambdaEngine::EntityCollisionInfo CollisionInfo0;
	const LambdaEngine::EntityCollisionInfo CollisionInfo1;
	const EAmmoType AmmoType;
	const LambdaEngine::ETeam Team;
	const uint32 Angle;
};
