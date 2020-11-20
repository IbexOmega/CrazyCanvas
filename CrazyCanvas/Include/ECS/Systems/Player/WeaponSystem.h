#pragma once
#include "ECS/System.h"
#include "ECS/Components/Player/ProjectileComponent.h"
#include "ECS/Components/Player/WeaponComponent.h"
#include "ECS/Components/Team/TeamComponent.h"
#include "ECS/Components/Multiplayer/PacketComponent.h"

#include "Game/ECS/Components/Rendering/ParticleEmitter.h"
#include "Game/ECS/Components/Rendering/MeshComponent.h"
#include "Game/ECS/Components/Rendering/AnimationComponent.h"

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
		{ RW,	PositionComponent::Type() },
		{ RW,	ScaleComponent::Type()},
		{ RW,	RotationComponent::Type() },
		{ RW,	VelocityComponent::Type()},
		{ R,	OffsetComponent::Type()},
		{ RW,	WeaponComponent::Type()},
		{ RW,	AnimationAttachedComponent::Type()},
		{ RW,	TeamComponent::Type() },
		{ RW,	ProjectileComponent::Type()},
		{ RW,	DynamicCollisionComponent::Type() },
		{ RW,	MeshComponent::Type() },
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
	virtual void Tick(LambdaEngine::Timestamp deltaTime) override
	{
		UNREFERENCED_VARIABLE(deltaTime);
	}

	virtual void Fire(LambdaEngine::Entity weaponEntity, WeaponComponent& weaponComponent, EAmmoType ammoType, const glm::vec3& position, const glm::vec3& velocity, uint8 playerTeam, uint32 angle);
	void CalculateWeaponFireProperties(LambdaEngine::Entity weaponEntity, glm::vec3& position, glm::vec3& velocity, uint8& playerTeam);

public:
	static bool Init();

	FORCEINLINE static WeaponSystem& GetInstance() 
	{ 
		VALIDATE(s_Instance != nullptr);
		return *s_Instance;
	}

protected:
	virtual bool InitInternal() = 0;

	void CreateBaseSystemRegistration(LambdaEngine::SystemRegistration& systemReg);

	void UpdateWeapon(WeaponComponent& weaponComponent, float32 dt);

	void StartReload(WeaponComponent& weaponComponent, PacketComponent<PacketPlayerAction>& packets);
	void AbortReload(WeaponComponent& weaponComponent);

	void OnProjectileHit(const LambdaEngine::EntityCollisionInfo& collisionInfo0, const LambdaEngine::EntityCollisionInfo& collisionInfo1);

	glm::vec3 CalculateZeroingDirection(
		const glm::vec3& weaponPos,
		const glm::vec3& playerPos,
		const glm::quat& playerDirection,
		float32 zeroingDistance);

protected:
	LambdaEngine::IDVector m_WeaponEntities;

private:
	static LambdaEngine::TUniquePtr<WeaponSystem> s_Instance;

private:
	float	m_ZeroDist	= 5.0f;
	float	m_YAngle	= 0.0f;
};
