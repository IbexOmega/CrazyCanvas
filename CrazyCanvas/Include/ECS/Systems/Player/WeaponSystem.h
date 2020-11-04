#pragma once
#include "ECS/System.h"
#include "ECS/Components/Player/ProjectileComponent.h"
#include "ECS/Components/Player/WeaponComponent.h"
#include "ECS/Components/Team/TeamComponent.h"
#include "ECS/Components/Multiplayer/PacketComponent.h"

#include "Game/ECS/Components/Rendering/ParticleEmitter.h"
#include "Game/ECS/Components/Rendering/MeshComponent.h"

#include "Math/Math.h"

#include "Multiplayer/Packet/PacketPlayerAction.h"
#include "Multiplayer/Packet/PacketPlayerActionResponse.h"

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
		{ RW, PositionComponent::Type()},
		{ RW, ScaleComponent::Type()},
		{ RW, RotationComponent::Type() },
		{ RW, VelocityComponent::Type()},
		{ RW, WeaponComponent::Type()},
		{ RW, TeamComponent::Type() },
		{ RW, ProjectileComponent::Type()},
		{ RW, DynamicCollisionComponent::Type() },
		{ RW, MeshComponent::Type() },
		{ RW, ParticleEmitterComponent::Type() },
	};
}

/*
* Helpers
*/

inline glm::vec3 CalculateWeaponPosition(
	const glm::vec3& playerPosition,
	const glm::quat& playerRotation,
	LambdaEngine::PositionComponent& weaponPositionComp,
	LambdaEngine::RotationComponent& weaponRotationComp,
	const LambdaEngine::OffsetComponent& weaponOffsetComp)
{
	glm::vec3 weaponPosition;
	glm::quat quatY = playerRotation;
	quatY.x = 0;
	quatY.z = 0;
	quatY = glm::normalize(quatY);
	weaponPositionComp.Position = playerPosition + quatY * weaponOffsetComp.Offset;
	weaponRotationComp.Quaternion = playerRotation;

	weaponPosition = weaponPositionComp.Position + LambdaEngine::GetForward(weaponRotationComp.Quaternion) * 0.2f;
	return weaponPosition;
}

/*
* WeaponSystem
*/

class WeaponSystem : public LambdaEngine::System
{
public:
	WeaponSystem()	= default;
	~WeaponSystem()	= default;

	virtual void FixedTick(LambdaEngine::Timestamp deltaTime) = 0;

	// Empty tick
	virtual void Tick(LambdaEngine::Timestamp deltaTime) override final
	{
		UNREFERENCED_VARIABLE(deltaTime);
	}

	virtual void Fire(
		EAmmoType ammoType,
		LambdaEngine::Entity weaponOwner,
		LambdaEngine::Entity weaponEntity,
		const glm::vec3& playerPos,
		const glm::quat& direction,
		const glm::vec3& playerVelocity);

public:
	static bool Init();
	static void Release();

	FORCEINLINE static WeaponSystem& GetInstance() 
	{ 
		VALIDATE(s_pInstance != nullptr);
		return *s_pInstance;
	}

protected:
	virtual bool InitInternal();

	virtual LambdaEngine::MeshComponent GetMeshComponent(EAmmoType ammoType, uint32 playerTeam)
	{
		UNREFERENCED_VARIABLE(ammoType);
		UNREFERENCED_VARIABLE(playerTeam);
		return LambdaEngine::MeshComponent();
	}

	// Returns true if we could fire
	virtual bool TryFire(
		EAmmoType ammoType,
		WeaponComponent& weaponComponent,
		PacketComponent<PacketPlayerAction>& packets,
		LambdaEngine::Entity weaponEntity,
		const glm::vec3& startPos,
		const glm::quat& direction,
		const glm::vec3& playerVelocity);

	void StartReload(WeaponComponent& weaponComponent, PacketComponent<PacketPlayerAction>& packets);
	void AbortReload(WeaponComponent& weaponComponent);

	void OnProjectileHit(const LambdaEngine::EntityCollisionInfo& collisionInfo0, const LambdaEngine::EntityCollisionInfo& collisionInfo1);

private:
	static WeaponSystem* s_pInstance;

protected:
	LambdaEngine::IDVector m_WeaponEntities;
};
