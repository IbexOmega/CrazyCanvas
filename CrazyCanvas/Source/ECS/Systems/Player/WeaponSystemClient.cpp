#include "ECS/Systems/Player/WeaponSystemClient.h"
#include "ECS/Components/Player/Player.h"
#include "ECS/ECSCore.h"

#include "Resources/Material.h"
#include "Resources/ResourceManager.h"

#include "Input/API/InputActionSystem.h"

#include "Math/Random.h"

#include "Match/Match.h"

#include "Resources/ResourceCatalog.h"

/*
* WeaponSystemClients
*/

void WeaponSystemClient::Tick(LambdaEngine::Timestamp deltaTime)
{
	using namespace LambdaEngine;

	UNREFERENCED_VARIABLE(deltaTime);

	ECSCore* pECS = ECSCore::GetInstance();

	const ComponentArray<WeaponComponent>*		pWeaponComponents				= pECS->GetComponentArray<WeaponComponent>();
	ComponentArray<PositionComponent>*			pPositionComponents				= pECS->GetComponentArray<PositionComponent>();
	ComponentArray<RotationComponent>*			pRotationComponents				= pECS->GetComponentArray<RotationComponent>();
	ComponentArray<ScaleComponent>*				pScaleComponent					= pECS->GetComponentArray<ScaleComponent>();

	for (Entity weaponEntity : m_WeaponEntities)
	{
		const WeaponComponent&				weaponComponent				= pWeaponComponents->GetConstData(weaponEntity);
		PositionComponent&					weaponPositionComponent		= pPositionComponents->GetData(weaponEntity);
		RotationComponent&					weaponRotationComponent		= pRotationComponents->GetData(weaponEntity);
		ScaleComponent&						weaponScaleComponent		= pScaleComponent->GetData(weaponEntity);

		const PositionComponent&			playerPositionComponent		= pPositionComponents->GetConstData(weaponComponent.WeaponOwner);
		const RotationComponent&			playerRotationComponent		= pRotationComponents->GetConstData(weaponComponent.WeaponOwner);
		const ScaleComponent&				playerScaleComponent		= pScaleComponent->GetConstData(weaponComponent.WeaponOwner);

		weaponPositionComponent.Position	= playerPositionComponent.Position;
		weaponRotationComponent.Quaternion	= playerRotationComponent.Quaternion;
		weaponScaleComponent.Scale			= playerScaleComponent.Scale;
	}
}

void WeaponSystemClient::FixedTick(LambdaEngine::Timestamp deltaTime)
{
	using namespace LambdaEngine;
	const float32 dt = (float32)deltaTime.AsSeconds();

	ECSCore* pECS = ECSCore::GetInstance();
	ComponentArray<PacketComponent<PacketPlayerAction>>*			pPlayerActionPackets	= pECS->GetComponentArray<PacketComponent<PacketPlayerAction>>();
	ComponentArray<PacketComponent<PacketPlayerActionResponse>>*	pPlayerResponsePackets	= pECS->GetComponentArray<PacketComponent<PacketPlayerActionResponse>>();
	ComponentArray<WeaponComponent>* pWeaponComponents										= pECS->GetComponentArray<WeaponComponent>();
	const ComponentArray<TeamComponent>* pTeamComponents									= pECS->GetComponentArray<TeamComponent>();
	
	// TODO: Check local response and maybe roll back
	for (Entity weaponEntity : m_WeaponEntities)
	{
		WeaponComponent& weaponComponent = pWeaponComponents->GetData(weaponEntity);
		Entity playerEntity = weaponComponent.WeaponOwner;

		const TeamComponent& teamComponent = pTeamComponents->GetConstData(playerEntity);

		// Foreign Players
		if (m_ForeignPlayerEntities.HasElement(playerEntity))
		{
			PacketComponent<PacketPlayerActionResponse>&	packets			= pPlayerResponsePackets->GetData(playerEntity);
			const TArray<PacketPlayerActionResponse>&		receivedPackets	= packets.GetPacketsReceived();
			for (const PacketPlayerActionResponse& response : receivedPackets)
			{
				if (response.FiredAmmo != EAmmoType::AMMO_TYPE_NONE)
				{
					Fire(weaponEntity,
						weaponComponent,
						response.FiredAmmo,
						response.WeaponPosition,
						response.WeaponVelocity,
						teamComponent.TeamIndex,
						response.Angle);
				}
			}

			continue;
		}

		// Local Player
		PacketComponent<PacketPlayerAction>& playerActions = pPlayerActionPackets->GetData(playerEntity);
		auto waterAmmo = weaponComponent.WeaponTypeAmmo.find(EAmmoType::AMMO_TYPE_WATER);
		VALIDATE(waterAmmo != weaponComponent.WeaponTypeAmmo.end())

		auto paintAmmo = weaponComponent.WeaponTypeAmmo.find(EAmmoType::AMMO_TYPE_PAINT);
		VALIDATE(paintAmmo != weaponComponent.WeaponTypeAmmo.end())
		
		const bool hasAmmo		= (waterAmmo->second.first > 0) || (paintAmmo->second.first > 0);
		const bool isReloading	= weaponComponent.ReloadClock > 0.0f;
		const bool onCooldown	= weaponComponent.CurrentCooldown > 0.0f;
		if (!hasAmmo && !isReloading)
		{
			StartReload(weaponComponent, playerActions);
		}

		// Reload if we are not reloading
		if (InputActionSystem::IsActive(EAction::ACTION_ATTACK_RELOAD) && !isReloading)
		{
			StartReload(weaponComponent, playerActions);
		}
		else if (!onCooldown) // If we did not hit the reload try and shoot
		{
			if (InputActionSystem::IsActive(EAction::ACTION_ATTACK_PRIMARY))
			{
				TryFire(EAmmoType::AMMO_TYPE_PAINT, weaponEntity);
			}
			else if (InputActionSystem::IsActive(EAction::ACTION_ATTACK_SECONDARY))
			{
				TryFire(EAmmoType::AMMO_TYPE_WATER, weaponEntity);
			}
		}

		// Update reload and cooldown timers
		UpdateWeapon(weaponComponent, dt);
	}
}

void WeaponSystemClient::Fire(LambdaEngine::Entity weaponEntity, WeaponComponent& weaponComponent, EAmmoType ammoType, const glm::vec3& position, const glm::vec3& velocity, uint8 playerTeam, uint32 angle)
{
	using namespace LambdaEngine;

	WeaponSystem::Fire(weaponEntity, weaponComponent, ammoType, position, velocity, playerTeam, angle);

	// Play gun fire and spawn particles
	ECSCore* pECS = ECSCore::GetInstance();
	const auto* pWeaponLocalComponents = pECS->GetComponentArray<WeaponLocalComponent>();

	// Play 2D sound if local player shooting else play 3D sound
	if (pWeaponLocalComponents->HasComponent(weaponEntity))
	{
		ISoundEffect2D* pSound = ResourceManager::GetSoundEffect2D(ResourceCatalog::WEAPON_SOUND_GUNFIRE_2D_GUID);
		pSound->PlayOnce(0.5f);
	}
	else
	{
		ISoundEffect3D* pSound = ResourceManager::GetSoundEffect3D(ResourceCatalog::WEAPON_SOUND_GUNFIRE_3D_GUID);
		pSound->PlayOnceAt(position, velocity, 0.25f, 1.0f);
	}
}

bool WeaponSystemClient::InitInternal()
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
		WeaponSystem::CreateBaseSystemRegistration(systemReg);

		systemReg.SubscriberRegistration.EntitySubscriptionRegistrations.PushBack(
			{
				.pSubscriber		= &m_ForeignPlayerEntities,
				.ComponentAccesses	=
				{
					{ NDA,	PlayerForeignComponent::Type() },
					{ RW,	PacketComponent<PacketPlayerActionResponse>::Type() }
				},
				.ComponentGroups = { &playerGroup }
			}
		);

		systemReg.SubscriberRegistration.EntitySubscriptionRegistrations.PushBack(
			{
				.pSubscriber		= &m_LocalPlayerEntities,
				.ComponentAccesses	=
				{
					{ NDA,	PlayerLocalComponent::Type() },
					{ RW,	PacketComponent<PacketPlayerAction>::Type() }
				},
				.ComponentGroups = { &playerGroup }
			}
		);

		RegisterSystem(TYPE_NAME(WeaponSystemClient), systemReg);
	}

	return true;
}

bool WeaponSystemClient::TryFire(EAmmoType ammoType, LambdaEngine::Entity weaponEntity)
{
	using namespace LambdaEngine;

	// Add cooldown
	ECSCore* pECS = ECSCore::GetInstance();
	WeaponComponent& weaponComponent	= pECS->GetComponent<WeaponComponent>(weaponEntity);
	weaponComponent.CurrentCooldown		= 1.0f / weaponComponent.FireRate;

	auto ammoState = weaponComponent.WeaponTypeAmmo.find(ammoType);
	VALIDATE(ammoState != weaponComponent.WeaponTypeAmmo.end());

	const bool hasAmmo = (ammoState->second.first > 0);
	if (Match::HasBegun() && hasAmmo)
	{
		// If we try to shoot when reloading we abort the reload
		const bool isReloading = weaponComponent.ReloadClock > 0.0f;
		if (isReloading)
		{
			AbortReload(weaponComponent);
		}

		//Calculate Weapon Fire Properties (Position, Velocity and Team)
		glm::vec3 firePosition;
		glm::vec3 fireVelocity;
		uint8 playerTeam;
		CalculateWeaponFireProperties(weaponEntity, firePosition, fireVelocity, playerTeam);

		uint32 angle = Random::UInt32(0, 360);

		// For creating entity
		Fire(weaponEntity, weaponComponent, ammoType, firePosition, fireVelocity, playerTeam, angle);

		// Send action to server
		PacketComponent<PacketPlayerAction>& packets = pECS->GetComponent<PacketComponent<PacketPlayerAction>>(weaponComponent.WeaponOwner);
		TQueue<PacketPlayerAction>& actions = packets.GetPacketsToSend();
		if (!actions.empty())
		{
			actions.back().FiredAmmo = ammoType;
			actions.back().Angle = angle;
		}

		return true;
	}
	else
	{
		// Play out of ammo
		ISoundEffect2D* pSound = ResourceManager::GetSoundEffect2D(ResourceCatalog::WEAPON_SOUND_OUTOFAMMO_2D_GUID);
		pSound->PlayOnce(1.0f, 1.0f);

		return false;
	}
}
