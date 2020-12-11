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

#include "Game/GameConsole.h"

#include "Resources/ResourceCatalog.h"

/*
* WeaponSystem
*/

LambdaEngine::TUniquePtr<WeaponSystem> WeaponSystem::s_Instance = nullptr;

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

void WeaponSystem::Release()
{
	s_Instance.Reset();
}

void WeaponSystem::CreateBaseSystemRegistration(LambdaEngine::SystemRegistration& systemReg)
{
	using namespace LambdaEngine;

	// Fill in Base System Registration
	{
		PlayerGroup playerGroup;
		playerGroup.Position.Permissions	= R;
		playerGroup.Scale.Permissions		= R;
		playerGroup.Rotation.Permissions	= R;
		playerGroup.Velocity.Permissions	= R;

		systemReg = {};
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
		systemReg.Phase = 2;
	}

	ConsoleCommand cmdZeroDist;
	cmdZeroDist.Init("proj_zero_dist", true);
	cmdZeroDist.AddDescription("Set the zero distance");
	cmdZeroDist.AddArg(Arg::EType::FLOAT);
	GameConsole::Get().BindCommand(cmdZeroDist, [this](GameConsole::CallbackInput& input)->void
		{
			m_ZeroDist = input.Arguments[0].Value.Float32;
		});

	ConsoleCommand cmdAngle;
	cmdAngle.Init("proj_angle", true);
	cmdAngle.AddDescription("Set the y angle distance");
	cmdAngle.AddArg(Arg::EType::FLOAT);
	GameConsole::Get().BindCommand(cmdAngle, [this](GameConsole::CallbackInput& input)->void
		{
			m_YAngle = input.Arguments[0].Value.Float32;
		});
}

void WeaponSystem::Fire(LambdaEngine::Entity weaponEntity, WeaponComponent& weaponComponent, EAmmoType ammoType, const glm::vec3& position, const glm::vec3& velocity, uint8 playerTeam, uint32 angle)
{
	using namespace LambdaEngine;

	UNREFERENCED_VARIABLE(weaponEntity);

	if (ammoType == EAmmoType::AMMO_TYPE_NONE)
	{
		return;
	}

	// Fire the gun
	auto pAmmoState = weaponComponent.WeaponTypeAmmo.find(ammoType);
	VALIDATE(pAmmoState != weaponComponent.WeaponTypeAmmo.end())

	pAmmoState->second.first--;

	// Fire event
	WeaponFiredEvent firedEvent(
		weaponComponent.WeaponOwner,
		ammoType,
		position,
		velocity,
		playerTeam,
		angle);
	firedEvent.Callback			= std::bind_front(&WeaponSystem::OnProjectileHit, this);
	EventQueue::SendEventImmediate(firedEvent);
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

			auto pWaterAmmo = weaponComponent.WeaponTypeAmmo.find(EAmmoType::AMMO_TYPE_WATER);
			VALIDATE(pWaterAmmo != weaponComponent.WeaponTypeAmmo.end())

			auto pPaintAmmo = weaponComponent.WeaponTypeAmmo.find(EAmmoType::AMMO_TYPE_PAINT);
			VALIDATE(pPaintAmmo != weaponComponent.WeaponTypeAmmo.end())

			pPaintAmmo->second.first = AMMO_CAPACITY;
			pWaterAmmo->second.first = AMMO_CAPACITY;

			//Reload Event
			WeaponReloadFinishedEvent reloadEvent(weaponComponent.WeaponOwner);
			EventQueue::SendEventImmediate(reloadEvent);
		}
	}
}

void WeaponSystem::OnProjectileHit(const LambdaEngine::EntityCollisionInfo& collisionInfo0, const LambdaEngine::EntityCollisionInfo& collisionInfo1)
{
	using namespace LambdaEngine;

	// Is this safe? Concurrency issues?
	ECSCore* pECS = ECSCore::GetInstance();
	const ComponentArray<TeamComponent>*		pTeamComponents			= pECS->GetComponentArray<TeamComponent>();
	const ComponentArray<ProjectileComponent>*	pProjectileComponents	= pECS->GetComponentArray<ProjectileComponent>();

	// Get ammotype
	EAmmoType ammoType	= EAmmoType::AMMO_TYPE_NONE;
	uint32 angle		= 0;
	if (pProjectileComponents->HasComponent(collisionInfo0.Entity))
	{
		const ProjectileComponent& projectilComp = pProjectileComponents->GetConstData(collisionInfo0.Entity);
		ammoType = projectilComp.AmmoType;
		angle = projectilComp.Angle;
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
	else if (!MultiplayerUtils::IsServer())
	{
		// Play Level Hit Sound
		ISoundEffect3D* pSound = ResourceManager::GetSoundEffect3D(ResourceCatalog::SOUND_EFFECT_SPLASH0_3D_GUID);
		pSound->PlayOnceAt(collisionInfo1.Position, glm::vec3(0.0f), 0.15f, 1.0f);

		levelHit = true;
	}

	if (levelHit || (ammoType == EAmmoType::AMMO_TYPE_WATER) || (!friendly && ammoType == EAmmoType::AMMO_TYPE_PAINT))
	{
		const ETeam team = (ETeam)projectileTeam;
		ProjectileHitEvent hitEvent(collisionInfo0, collisionInfo1, ammoType, team, angle);
		EventQueue::SendEventImmediate(hitEvent);
	}
}

void WeaponSystem::CalculateWeaponFireProperties(LambdaEngine::Entity weaponEntity, glm::vec3& position, glm::vec3& velocity, uint8& playerTeam)
{
	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();
	WeaponComponent& weaponComponent = pECS->GetComponent<WeaponComponent>(weaponEntity);

	const Entity weaponOwner = weaponComponent.WeaponOwner;
	PositionComponent playerPositionComponent;
	RotationComponent playerRotationComponent;

	if (!pECS->GetConstComponentIf<PositionComponent>(weaponOwner, playerPositionComponent) ||
		!pECS->GetConstComponentIf<RotationComponent>(weaponOwner, playerRotationComponent))
	{
		return;
	}

	const OffsetComponent&		weaponOffsetComponent	= pECS->GetConstComponent<OffsetComponent>(weaponEntity);

	const glm::vec3 playerForwardDirection	= GetForward(playerRotationComponent.Quaternion);

	constexpr const float PROJECTILE_INITAL_SPEED = 30.0f;

	//Don't use weapon position/rotation because it now depends on animation, only use player data instead.
	const glm::vec3 right = glm::normalize(GetRight(playerRotationComponent.Quaternion));
	const glm::vec3 up = glm::normalize(g_DefaultUp);
	const glm::vec3 forward = glm::normalize(GetForward(playerRotationComponent.Quaternion));

	position = playerPositionComponent.Position +
		up * weaponOffsetComponent.Offset.y +
		right * weaponOffsetComponent.Offset.x +
		forward * weaponOffsetComponent.Offset.z;

	const glm::vec3 zeroingDirection	= CalculateZeroingDirection(position, playerPositionComponent.Position, playerRotationComponent.Quaternion, m_ZeroDist);

	velocity		= zeroingDirection * PROJECTILE_INITAL_SPEED;
	playerTeam		= pECS->GetConstComponent<TeamComponent>(weaponOwner).TeamIndex;
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

	WeaponReloadStartedEvent reloadStartedEvent(weaponComponent.WeaponOwner);
	LambdaEngine::EventQueue::SendEventImmediate(reloadStartedEvent);
}

void WeaponSystem::AbortReload(WeaponComponent& weaponComponent)
{
	weaponComponent.ReloadClock = 0;

	WeaponReloadCanceledEvent reloadCancelEvent(weaponComponent.WeaponOwner);
	LambdaEngine::EventQueue::SendEventImmediate(reloadCancelEvent);
}

glm::vec3 WeaponSystem::CalculateZeroingDirection(
	const glm::vec3& weaponPos,
	const glm::vec3& playerPos,
	const glm::quat& playerDirection,
	float32 zeroingDistance)
{
	using namespace LambdaEngine;
	glm::vec3 zeroPoint = glm::vec3{ playerPos.x, weaponPos.y, playerPos.z } + glm::normalize(GetForward(playerDirection)) * zeroingDistance;
	glm::vec3 fireDirection = glm::normalize(zeroPoint - weaponPos);

	// adjusts m_YAngle to be less impactful when aiming up or down.
	float angleFactor = 1.0f - abs(glm::dot(g_DefaultUp, fireDirection)) * 0.5f;
	return glm::rotate(fireDirection, glm::radians(m_YAngle * angleFactor), glm::cross(fireDirection, GetUp(playerDirection)));
}
