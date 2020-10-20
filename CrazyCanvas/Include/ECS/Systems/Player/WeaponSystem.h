#pragma once
#include "ECS/System.h"
#include "ECS/Components/Player/ProjectileComponent.h"

#include "Game/ECS/Components/Rendering/MeshComponent.h"

#include "Math/Math.h"

namespace LambdaEngine
{
	struct EntityCollisionInfo;
}

struct WeaponComponent;

/*
* WeaponProperties
*/

struct WeaponProperties
{
	float32	FireRate		= 4.0f;
	int32	AmmoCapacity	= 50;
	EAmmoType AmmoType		= EAmmoType::AMMO_TYPE_PAINT;
};

/*
* WeaponComponent
*/

class WeaponSystem : public LambdaEngine::System
{
public:
	WeaponSystem() = default;
	~WeaponSystem() = default;

	bool Init();

	void Tick(LambdaEngine::Timestamp deltaTime) override final;

private:
	void Fire(EAmmoType ammoType, WeaponComponent& weaponComponent, const glm::vec3& startPos, const glm::quat& direction, const glm::vec3& playerVelocity);
	void OnProjectileHit(const LambdaEngine::EntityCollisionInfo& collisionInfo0, const LambdaEngine::EntityCollisionInfo& collisionInfo1);

	void StartReload(WeaponComponent& weaponComponent);
	void AbortReload(WeaponComponent& weaponComponent);

	void TryFire(EAmmoType ammoType, WeaponComponent& weaponComponent, const glm::vec3& startPos, const glm::quat& direction, const glm::vec3& playerVelocity);

private:
	LambdaEngine::IDVector m_WeaponEntities;
	LambdaEngine::IDVector m_PlayerEntities;

	// Rendering resources for projectiles
	LambdaEngine::MeshComponent m_PaintProjectileMeshComponent;
	LambdaEngine::MeshComponent m_WaterProjectileMeshComponent;

	GUID_Lambda m_GunFireGUID	= GUID_NONE;
	GUID_Lambda m_OutOfAmmoGUID	= GUID_NONE;
};
