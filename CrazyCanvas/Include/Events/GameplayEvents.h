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
	inline PlayerDiedEvent(const LambdaEngine::Entity killedEntity, const glm::vec3 position)
		: Event()
		, KilledEntity(killedEntity)
		, Position(position)
	{
	}

	DECLARE_EVENT_TYPE(PlayerDiedEvent);

	virtual LambdaEngine::String ToString() const
	{
		using namespace LambdaEngine;
		return
			"Player Died. EntitiyID=" +
			std::to_string(KilledEntity) +
			"Position=[" +
			std::to_string(Position.x) +
			", " +
			std::to_string(Position.y) +
			", " +
			std::to_string(Position.z) +
			"]";
	}

	const LambdaEngine::Entity KilledEntity;
	const glm::vec3 Position;
};

/*
* PlayerConnectedEvent
*/

struct PlayerConnectedEvent : public LambdaEngine::Event
{
public:
	inline PlayerConnectedEvent(const LambdaEngine::Entity newEntity, const glm::vec3 position)
		: Event()
		, NewEntity(newEntity)
		, Position(position)
	{
	}

	DECLARE_EVENT_TYPE(PlayerConnectedEvent);

	virtual LambdaEngine::String ToString() const
	{
		using namespace LambdaEngine;
		return
			"Player Connected. EntitiyID=" +
			std::to_string(NewEntity) +
			"Position=[" +
			std::to_string(Position.x) +
			", " +
			std::to_string(Position.y) +
			", " +
			std::to_string(Position.z) +
			"]";
	}

	const LambdaEngine::Entity NewEntity;
	const glm::vec3 Position;
};

/*
* PlayerHitEvent
*/

struct PlayerHitEvent : public LambdaEngine::Event
{
public:
	inline PlayerHitEvent(const LambdaEngine::Entity hitEntity, const glm::vec3 hitPosition, const bool isLocal)
		: Event()
		, HitEntity(hitEntity)
		, HitPosition(hitPosition)
		, IsLocal(isLocal)
	{
	}

	DECLARE_EVENT_TYPE(PlayerHitEvent);

	virtual LambdaEngine::String ToString() const
	{
		using namespace LambdaEngine;
		return
			"Local player got hit. EntitiyID=" +
			std::to_string(HitEntity) +
			"Position=[" +
			std::to_string(HitPosition.x) +
			", " +
			std::to_string(HitPosition.y) +
			", " +
			std::to_string(HitPosition.z) +
			"] Local=" + 
			(IsLocal ? "true" : "false");
	}

	const LambdaEngine::Entity HitEntity;
	const glm::vec3 HitPosition;
	const bool IsLocal;
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