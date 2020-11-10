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

#include "Containers/TUniquePtr.h"

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
		{ RW, PositionComponent::Type() },
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

	virtual void Fire(EAmmoType ammoType, LambdaEngine::Entity weaponEntity);
	// Returns true if we could fire
	virtual bool TryFire(EAmmoType ammoType, LambdaEngine::Entity weaponEntity);

public:
	static bool Init();

	FORCEINLINE static WeaponSystem& GetInstance() 
	{ 
		VALIDATE(s_Instance != nullptr);
		return *s_Instance;
	}

protected:
	virtual bool InitInternal();

	virtual LambdaEngine::MeshComponent GetMeshComponent(EAmmoType ammoType, uint32 playerTeam)
	{
		UNREFERENCED_VARIABLE(ammoType);
		UNREFERENCED_VARIABLE(playerTeam);
		return LambdaEngine::MeshComponent();
	}

	void UpdateWeapon(WeaponComponent& weaponComponent, float32 dt);

	void StartReload(WeaponComponent& weaponComponent, PacketComponent<PacketPlayerAction>& packets);
	void AbortReload(WeaponComponent& weaponComponent);

	void OnProjectileHit(const LambdaEngine::EntityCollisionInfo& collisionInfo0, const LambdaEngine::EntityCollisionInfo& collisionInfo1);

protected:
	LambdaEngine::IDVector m_WeaponEntities;

private:
	static LambdaEngine::TUniquePtr<WeaponSystem> s_Instance;
};
