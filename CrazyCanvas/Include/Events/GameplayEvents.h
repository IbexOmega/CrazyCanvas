#pragma once
#include "Application/API/Events/Event.h"

#include "ECS/Entity.h"
#include "ECS/Components/Player/ProjectileComponent.h"

#include "Game/ECS/Components/Rendering/MeshComponent.h"

#include "Math/Math.h"

#include "Game/ECS/Systems/Physics/PhysicsSystem.h"

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
		const uint8 teamIndex,
		const uint32 angle)
		: Event()
		, WeaponOwnerEntity(weaponOwnerEntity)
		, AmmoType(ammoType)
		, Position(position)
		, InitialVelocity(initialVelocity)
		, TeamIndex(teamIndex)
		, Angle(angle)
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
	const uint8 TeamIndex;
	const uint32 Angle;
	LambdaEngine::CollisionCallback Callback;
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

struct WeaponReloadStartedEvent : public LambdaEngine::Event
{
public:
	inline WeaponReloadStartedEvent(const LambdaEngine::Entity weaponOwnerEntity) :
		WeaponOwnerEntity(weaponOwnerEntity)
	{
	}

	virtual LambdaEngine::String ToString() const
	{
		using namespace LambdaEngine;
		return String("Player Started to reload a weapon. EntitiyID=%u", WeaponOwnerEntity);
	}

	DECLARE_EVENT_TYPE(WeaponReloadStartedEvent);

	const LambdaEngine::Entity WeaponOwnerEntity;
};

struct WeaponReloadCanceledEvent : public LambdaEngine::Event
{
public:
	inline WeaponReloadCanceledEvent(const LambdaEngine::Entity weaponOwnerEntity) :
		WeaponOwnerEntity(weaponOwnerEntity)
	{
	}

	virtual LambdaEngine::String ToString() const
	{
		using namespace LambdaEngine;
		return String("Player Canceled to reload a weapon. EntitiyID=%u", WeaponOwnerEntity);
	}

	DECLARE_EVENT_TYPE(WeaponReloadCanceledEvent);

	const LambdaEngine::Entity WeaponOwnerEntity;
};

struct SpectatePlayerEvent : public LambdaEngine::Event
{
public:
	inline SpectatePlayerEvent(const LambdaEngine::String& name) :
		PlayerName(name)
	{
	}

	virtual LambdaEngine::String ToString() const
	{
		using namespace LambdaEngine;
		return String("Player Spectating player " +  PlayerName);
	}

	DECLARE_EVENT_TYPE(SpectatePlayerEvent);

	const LambdaEngine::String& PlayerName;
};
