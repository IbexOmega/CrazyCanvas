#pragma once
#include "ECS/System.h"
#include "ECS/Components/Player/ProjectileComponent.h"
#include "ECS/Components/Player/WeaponComponent.h"
#include "ECS/Components/Team/TeamComponent.h"

#include "Game/ECS/Components/Rendering/ParticleEmitter.h"
#include "Game/ECS/Components/Rendering/MeshComponent.h"

#include "Math/Math.h"

#include "Multiplayer/Packet/PlayerAction.h"
#include "Multiplayer/Packet/PlayerActionResponse.h"

namespace LambdaEngine
{
	struct EntityCollisionInfo;
}

/*
* WeaponProperties
*/

struct WeaponProperties
{
	float32	FireRate		= 4.0f;
	int32	AmmoCapacity	= 50;
	EAmmoType AmmoType		= EAmmoType::AMMO_TYPE_PAINT;
};

inline LambdaEngine::TArray<LambdaEngine::ComponentAccess> GetFireProjectileComponentAccesses()
{
	using namespace LambdaEngine;
	return
	{
		{ RW, PositionComponent::Type()}, { RW, ScaleComponent::Type()}, { RW, RotationComponent::Type() },
		{ RW, VelocityComponent::Type()}, { RW, WeaponComponent::Type()}, { RW, TeamComponent::Type() },
		{ RW, ProjectileComponent::Type()}, { RW, DynamicCollisionComponent::Type() }, { RW, MeshComponent::Type() }
	};
}

/*
* WeaponSystem
*/

class WeaponSystem : public LambdaEngine::System
{
public:
	bool Init();

	void FixedTick(LambdaEngine::Timestamp deltaTime);

	// Empty tick
	virtual void Tick(LambdaEngine::Timestamp deltaTime) override final
	{
		UNREFERENCED_VARIABLE(deltaTime);
	}

	void TryFire(
		EAmmoType ammoType,
		WeaponComponent& weaponComponent,
		LambdaEngine::Entity weaponEntity,
		const glm::vec3& startPos, 
		const glm::quat& direction, 
		const glm::vec3& playerVelocity);

public:
	static WeaponSystem& GetInstance() { return s_Instance; }

private:
	WeaponSystem() = default;
	~WeaponSystem() = default;

	void Fire(
		EAmmoType ammoType,
		LambdaEngine::Entity weaponOwner,
		LambdaEngine::Entity weaponEntity,
		const glm::vec3& playerPos,
		const glm::quat& direction,
		const glm::vec3& playerVelocity);

	void TryFire(
		EAmmoType ammoType,
		WeaponComponent& weaponComponent,
		PacketComponent<PlayerAction>& packets,
		LambdaEngine::Entity weaponEntity,
		const glm::vec3& startPos,
		const glm::quat& direction,
		const glm::vec3& playerVelocity);

	void StartReload(WeaponComponent& weaponComponent, PacketComponent<PlayerAction>& packets);
	void AbortReload(WeaponComponent& weaponComponent);

	void OnProjectileHit(const LambdaEngine::EntityCollisionInfo& collisionInfo0, const LambdaEngine::EntityCollisionInfo& collisionInfo1);

private:
	static WeaponSystem s_Instance;

private:
	LambdaEngine::IDVector m_WeaponEntities;
	LambdaEngine::IDVector m_RemotePlayerEntities;
	LambdaEngine::IDVector m_LocalPlayerEntities;
	LambdaEngine::IDVector m_ForeignPlayerEntities;

	// Rendering resources for projectiles
	LambdaEngine::MeshComponent m_RedPaintProjectileMeshComponent;
	LambdaEngine::MeshComponent m_BluePaintProjectileMeshComponent;
	LambdaEngine::MeshComponent m_WaterProjectileMeshComponent;

	GUID_Lambda m_GunFireGUID	= GUID_NONE;
	GUID_Lambda m_OutOfAmmoGUID	= GUID_NONE;
};
