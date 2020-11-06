#include "ECS/Systems/Player/WeaponSystem.h"
#include "ECS/Systems/Player/WeaponSystemClient.h"
#include "ECS/Systems/Player/WeaponSystemServer.h"
#include "ECS/Components/Player/Player.h"
#include "ECS/ECSCore.h"

#include "Application/API/Events/EventQueue.h"

#include "Game/ECS/Systems/Physics/PhysicsSystem.h"
#include "Game/Multiplayer/MultiplayerUtils.h"
#include "Game/ECS/Components/Physics/Transform.h"

#include "Events/GameplayEvents.h"

#include "Input/API/InputActionSystem.h"

#include "Physics/PhysicsEvents.h"
#include "Physics/CollisionGroups.h"

#include "Resources/Material.h"
#include "Resources/ResourceManager.h"

#include "Teams/TeamHelper.h"

#include "Match/Match.h"

/*
* WeaponSystem
*/

LambdaEngine::TUniquePtr<WeaponSystem> WeaponSystem::s_Instance = nullptr;

void WeaponSystem::Fire(EAmmoType ammoType, LambdaEngine::Entity weaponEntity)
{
	using namespace LambdaEngine;

	if (ammoType == EAmmoType::AMMO_TYPE_NONE)
	{
		return;
	}

	ECSCore* pECS = ECSCore::GetInstance();
	WeaponComponent& weaponComponent = pECS->GetComponent<WeaponComponent>(weaponEntity);

	const Entity weaponOwner = weaponComponent.WeaponOwner;
	PositionComponent&			weaponPositionComponent	= pECS->GetComponent<PositionComponent>(weaponEntity);
	RotationComponent&			weaponRotationComponent	= pECS->GetComponent<RotationComponent>(weaponEntity);
	const OffsetComponent&		weaponOffsetComponent	= pECS->GetConstComponent<OffsetComponent>(weaponEntity);
	const PositionComponent&	positionComponent = pECS->GetConstComponent<PositionComponent>(weaponOwner);
	const RotationComponent&	rotationComponent = pECS->GetConstComponent<RotationComponent>(weaponOwner);
	const VelocityComponent&	velocityComponent = pECS->GetConstComponent<VelocityComponent>(weaponOwner);
	const glm::quat& direction		= rotationComponent.Quaternion;
	const glm::vec3& playerPos		= positionComponent.Position;
	const glm::vec3& playerVelocity	= velocityComponent.Velocity;

	{
		glm::quat quatY = direction;
		quatY.x = 0;
		quatY.z = 0;
		quatY = glm::normalize(quatY);
		weaponPositionComponent.Position	= playerPos + quatY * weaponOffsetComponent.Offset;
		weaponRotationComponent.Quaternion	= direction;
	}

	constexpr const float PROJECTILE_INITAL_SPEED = 13.0f;
	const glm::vec3 weaponPos		= weaponPositionComponent.Position + GetForward(weaponRotationComponent.Quaternion) * 0.2f;
	const glm::vec3 directionVec	= GetForward(glm::normalize(direction));
	const glm::vec3 initialVelocity	= playerVelocity + directionVec * PROJECTILE_INITAL_SPEED;
	const uint32	playerTeam		= pECS->GetConstComponent<TeamComponent>(weaponOwner).TeamIndex;

	// Fire event
	WeaponFiredEvent firedEvent(
		weaponOwner,
		ammoType,
		weaponPos,
		initialVelocity,
		direction,
		playerTeam);
	firedEvent.Callback			= std::bind_front(&WeaponSystem::OnProjectileHit, this);
	firedEvent.MeshComponent	= GetMeshComponent(ammoType, playerTeam);
	EventQueue::SendEventImmediate(firedEvent);

	// Fire the gun
	auto ammoState = weaponComponent.WeaponTypeAmmo.find(ammoType);
	VALIDATE(ammoState != weaponComponent.WeaponTypeAmmo.end())

	ammoState->second.first--;
}

bool WeaponSystem::Init()
{
	using namespace LambdaEngine;

	if (MultiplayerUtils::IsServer())
	{
		s_Instance = DBG_NEW WeaponSystemServer();
	}
	else
	{
		s_Instance = DBG_NEW WeaponSystemClient();
	}

	return s_Instance->InitInternal();
}

bool WeaponSystem::InitInternal()
{
	using namespace LambdaEngine;

	// Register system
	{
		PlayerGroup playerGroup;
		playerGroup.Position.Permissions	= R;
		playerGroup.Scale.Permissions		= R;
		playerGroup.Rotation.Permissions	= R;
		playerGroup.Velocity.Permissions	= R;

		SystemRegistration systemReg = {};
		systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
		{
			{
				.pSubscriber		= &m_WeaponEntities,
				.ComponentAccesses	=
				{
					{ RW, WeaponComponent::Type() },
				}
			},
		};

		systemReg.SubscriberRegistration.AdditionalAccesses = GetFireProjectileComponentAccesses();
		systemReg.Phase = 1;

		RegisterSystem(TYPE_NAME(WeaponSystem), systemReg);
	}

	return true;
}

bool WeaponSystem::TryFire(EAmmoType ammoType, LambdaEngine::Entity weaponEntity)
{
	using namespace LambdaEngine;

	// Add cooldown
	ECSCore* pECS = ECSCore::GetInstance();
	WeaponComponent& weaponComponent	= pECS->GetComponent<WeaponComponent>(weaponEntity);
	weaponComponent.CurrentCooldown		= 1.0f / weaponComponent.FireRate;

	auto ammoState = weaponComponent.WeaponTypeAmmo.find(ammoType);
	VALIDATE(ammoState != weaponComponent.WeaponTypeAmmo.end());

	const bool hasAmmo = (ammoState->second.first > 0);
	if (hasAmmo)
	{
		// If we try to shoot when reloading we abort the reload
		const bool isReloading = weaponComponent.ReloadClock > 0.0f;
		if (isReloading)
		{
			AbortReload(weaponComponent);
		}

		// For creating entity
		Fire(ammoType, weaponEntity);
		return true;
	}
	else
	{
		return false;
	}
}

void WeaponSystem::UpdateWeapon(WeaponComponent& weaponComponent, float32 dt)
{
	using namespace LambdaEngine;

	const bool isReloading	= weaponComponent.ReloadClock > 0.0f;
	const bool onCooldown	= weaponComponent.CurrentCooldown > 0.0f;

	if (onCooldown)
	{
		weaponComponent.CurrentCooldown -= dt;
	}

	if (isReloading)
	{
		weaponComponent.ReloadClock -= dt;
		if (weaponComponent.ReloadClock < 0.0f)
		{
			weaponComponent.ReloadClock = 0.0f;

			auto waterAmmo = weaponComponent.WeaponTypeAmmo.find(EAmmoType::AMMO_TYPE_WATER);
			VALIDATE(waterAmmo != weaponComponent.WeaponTypeAmmo.end())

			auto paintAmmo = weaponComponent.WeaponTypeAmmo.find(EAmmoType::AMMO_TYPE_PAINT);
			VALIDATE(paintAmmo != weaponComponent.WeaponTypeAmmo.end())

			paintAmmo->second.first = AMMO_CAPACITY;
			waterAmmo->second.first = AMMO_CAPACITY;

			//Reload Event
			WeaponReloadFinishedEvent reloadEvent(weaponComponent.WeaponOwner);
			EventQueue::SendEventImmediate(reloadEvent);
		}
	}
}

void WeaponSystem::OnProjectileHit(const LambdaEngine::EntityCollisionInfo& collisionInfo0, const LambdaEngine::EntityCollisionInfo& collisionInfo1)
{
	using namespace LambdaEngine;

	LOG_INFO("Projectile hit: collisionInfo0: %d, collisionInfo1: %d", collisionInfo0.Entity, collisionInfo1.Entity);

	// Is this safe? Concurrency issues?
	ECSCore* pECS = ECSCore::GetInstance();
	const ComponentArray<TeamComponent>*		pTeamComponents			= pECS->GetComponentArray<TeamComponent>();
	const ComponentArray<ProjectileComponent>*	pProjectileComponents	= pECS->GetComponentArray<ProjectileComponent>();

	// Get ammotype
	EAmmoType ammoType = EAmmoType::AMMO_TYPE_NONE;
	if (pProjectileComponents->HasComponent(collisionInfo0.Entity))
	{
		const ProjectileComponent& projectilComp = pProjectileComponents->GetConstData(collisionInfo0.Entity);
		ammoType = projectilComp.AmmoType;
	}

	// Always destroy projectile but do not send event if we hit a friend
	pECS->RemoveEntity(collisionInfo0.Entity);

	// Disable friendly fire
	bool friendly	= false;
	bool levelHit	= false;
	const uint32 projectileTeam = pTeamComponents->GetConstData(collisionInfo0.Entity).TeamIndex;
	if (pTeamComponents->HasComponent(collisionInfo1.Entity))
	{
		// Friendly if we hit teammate and the type is paint
		const uint32 otherEntityTeam = pTeamComponents->GetConstData(collisionInfo1.Entity).TeamIndex;
		if (projectileTeam == otherEntityTeam)
		{
			friendly = true;
		}
	}
	else
	{
		levelHit = true;
	}

	if (levelHit || (friendly && ammoType == EAmmoType::AMMO_TYPE_WATER) || (!friendly && ammoType == EAmmoType::AMMO_TYPE_PAINT))
	{
		const ETeam team = (projectileTeam == 0) ? ETeam::BLUE : ETeam::RED;
		ProjectileHitEvent hitEvent(collisionInfo0, collisionInfo1, ammoType, team);
		EventQueue::SendEventImmediate(hitEvent);
	}
}

void WeaponSystem::StartReload(WeaponComponent& weaponComponent, PacketComponent<PacketPlayerAction>& packets)
{
	using namespace LambdaEngine;

	// Send action to server
	TQueue<PacketPlayerAction>& actions = packets.GetPacketsToSend();
	if (!actions.empty())
	{
		actions.back().StartedReload = true;
	}

	weaponComponent.ReloadClock = weaponComponent.ReloadTime;
}

void WeaponSystem::AbortReload(WeaponComponent& weaponComponent)
{
	weaponComponent.ReloadClock = 0;
}
