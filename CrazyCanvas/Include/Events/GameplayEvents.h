#pragma once
#include "Application/API/Events/Event.h"

#include "ECS/Entity.h"
#include "ECS/Components/Player/ProjectileComponent.h"

#include "Game/ECS/Components/Rendering/MeshComponent.h"

#include "Math/Math.h"

#include "Game/ECS/Systems/Physics/PhysicsSystem.h"

/*
* PlayerDiedEvent
*/

struct PlayerDiedEvent : public LambdaEngine::Event
{
public:
	inline PlayerDiedEvent(const LambdaEngine::Entity killedEntity)
		: Event()
		, KilledEntity(killedEntity)
	{
	}

	DECLARE_EVENT_TYPE(PlayerDiedEvent);

	virtual LambdaEngine::String ToString() const
	{
		using namespace LambdaEngine;
		return String("Player Died. EntitiyID=%u", KilledEntity);
	}

	const LambdaEngine::Entity KilledEntity;
};

/*
* WeaponFiredEvent
*/

struct WeaponFiredEvent : public LambdaEngine::Event
{
public:
	inline WeaponFiredEvent(
		const LambdaEngine::Entity weaponOwnerEntity, 
		const EAmmoType ammoType, 
		const glm::vec3& position, 
		const glm::vec3& initialVelocity,
		const glm::quat& direction,
		const uint32 teamIndex)
		: Event()
		, WeaponOwnerEntity(weaponOwnerEntity)
		, AmmoType(ammoType)
		, Position(position)
		, InitialVelocity(initialVelocity)
		, Direction(direction)
		, TeamIndex(teamIndex)
	{
	}

	DECLARE_EVENT_TYPE(WeaponFiredEvent);

	virtual LambdaEngine::String ToString() const
	{
		using namespace LambdaEngine;
		return String("Player fired a weapon. EntitiyID=%u", WeaponOwnerEntity);
	}

	const LambdaEngine::Entity WeaponOwnerEntity;
	const EAmmoType AmmoType;
	const glm::vec3 Position;
	const glm::vec3 InitialVelocity;
	const glm::quat Direction;
	const uint32 TeamIndex;
	LambdaEngine::CollisionCallback Callback;
	LambdaEngine::MeshComponent		MeshComponent;
};

/*
* WeaponReloadFinishedEvent
*/

struct WeaponReloadFinishedEvent : public LambdaEngine::Event
{
public:
	inline WeaponReloadFinishedEvent(const LambdaEngine::Entity weaponOwnerEntity) : 
		WeaponOwnerEntity(weaponOwnerEntity)
	{
	}

	virtual LambdaEngine::String ToString() const
	{
		using namespace LambdaEngine;
		return String("Player finished to reload a weapon. EntitiyID=%u", WeaponOwnerEntity);
	}

	DECLARE_EVENT_TYPE(WeaponReloadFinishedEvent);

	const LambdaEngine::Entity WeaponOwnerEntity;
};