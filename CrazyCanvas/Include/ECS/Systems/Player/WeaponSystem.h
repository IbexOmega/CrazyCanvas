#pragma once
#include "ECS/System.h"
#include "ECS/Components/Player/ProjectileComponent.h"
#include "ECS/Components/Player/WeaponComponent.h"

#include "Game/ECS/Components/Rendering/MeshComponent.h"

#include "Math/Math.h"

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

/*
* WeaponComponent
*/

class WeaponSystem : public LambdaEngine::System
{
public:
	WeaponSystem() = default;
	~WeaponSystem() = default;

	bool Init();

	void FixedTick(LambdaEngine::Timestamp deltaTime);

	// Empty tick
	virtual void Tick(LambdaEngine::Timestamp deltaTime) override final
	{
		UNREFERENCED_VARIABLE(deltaTime);
	}

private:
	void Fire(
		EAmmoType ammoType, 
		WeaponComponent& weaponComponent, 
		PacketComponent<WeaponFiredPacket>& packets,
		const glm::vec3& playerPos, 
		const glm::quat& direction, 
		const glm::vec3& playerVelocity);

	void TryFire(
		EAmmoType ammoType,
		WeaponComponent& weaponComponent,
		PacketComponent<WeaponFiredPacket>& packets,
		const glm::vec3& startPos, 
		const glm::quat& direction, 
		const glm::vec3& playerVelocity);

	void StartReload(WeaponComponent& weaponComponent);
	void AbortReload(WeaponComponent& weaponComponent);

	void OnProjectileHit(const LambdaEngine::EntityCollisionInfo& collisionInfo0, const LambdaEngine::EntityCollisionInfo& collisionInfo1);

private:
	LambdaEngine::IDVector m_WeaponEntities;
	LambdaEngine::IDVector m_PlayerEntities;

	// Rendering resources for projectiles
	LambdaEngine::MeshComponent m_PaintProjectileMeshComponent;
	LambdaEngine::MeshComponent m_WaterProjectileMeshComponent;

	GUID_Lambda m_GunFireGUID	= GUID_NONE;
	GUID_Lambda m_OutOfAmmoGUID	= GUID_NONE;
};
