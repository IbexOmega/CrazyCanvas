#include "ECS/Systems/Player/WeaponSystemClient.h"
#include "ECS/Components/Player/Player.h"
#include "ECS/ECSCore.h"

#include "Resources/Material.h"
#include "Resources/ResourceManager.h"

#include "Input/API/InputActionSystem.h"

/*
* WeaponSystemClients
*/

void WeaponSystemClient::FixedTick(LambdaEngine::Timestamp deltaTime)
{
	using namespace LambdaEngine;
	const float32 dt = (float32)deltaTime.AsSeconds();

	ECSCore* pECS = ECSCore::GetInstance();
	ComponentArray<PacketComponent<PacketPlayerAction>>*			pPlayerActionPackets	= pECS->GetComponentArray<PacketComponent<PacketPlayerAction>>();
	ComponentArray<PacketComponent<PacketPlayerActionResponse>>*	pPlayerResponsePackets	= pECS->GetComponentArray<PacketComponent<PacketPlayerActionResponse>>();
	ComponentArray<WeaponComponent>* pWeaponComponents = pECS->GetComponentArray<WeaponComponent>();
	
	// TODO: Check local response and maybe roll back
	for (Entity weaponEntity : m_WeaponEntities)
	{
		WeaponComponent& weaponComponent = pWeaponComponents->GetData(weaponEntity);
		Entity playerEntity = weaponComponent.WeaponOwner;

		// Foreign Players
		if (m_ForeignPlayerEntities.HasElement(playerEntity))
		{
			PacketComponent<PacketPlayerActionResponse>&	packets			= pPlayerResponsePackets->GetData(playerEntity);
			const TArray<PacketPlayerActionResponse>&		receivedPackets	= packets.GetPacketsReceived();
			for (const PacketPlayerActionResponse& response : receivedPackets)
			{
				if (response.FiredAmmo != EAmmoType::AMMO_TYPE_NONE)
				{
					Fire(response.FiredAmmo, weaponEntity);
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

void WeaponSystemClient::Fire(EAmmoType ammoType, LambdaEngine::Entity weaponEntity)
{
	using namespace LambdaEngine;

	WeaponSystem::Fire(ammoType, weaponEntity);

	ECSCore* pECS = ECSCore::GetInstance();
	const WeaponComponent&		weaponComponent		= pECS->GetConstComponent<WeaponComponent>(weaponEntity);
	const PositionComponent&	positionComponent	= pECS->GetConstComponent<PositionComponent>(weaponComponent.WeaponOwner);
	const VelocityComponent&	velocityComponent	= pECS->GetConstComponent<VelocityComponent>(weaponComponent.WeaponOwner);

	// Play gun fire and spawn particles
	ISoundEffect3D* m_pSound = ResourceManager::GetSoundEffect3D(m_GunFireGUID);
	m_pSound->PlayOnceAt(positionComponent.Position, velocityComponent.Velocity, 0.2f, 1.0f);

	ParticleEmitterComponent& emitterComp = pECS->GetComponent<ParticleEmitterComponent>(weaponEntity);
	emitterComp.Active = true;

	LOG_MESSAGE("FIRE");
}

bool WeaponSystemClient::InitInternal()
{
	using namespace LambdaEngine;

	if (!WeaponSystem::InitInternal())
	{
		return false;
	}

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
				.pSubscriber		= &m_ForeignPlayerEntities,
				.ComponentAccesses	=
				{
					{ NDA,	PlayerForeignComponent::Type() },
					{ RW,	PacketComponent<PacketPlayerActionResponse>::Type() }
				},
				.ComponentGroups = { &playerGroup }
			},
			{
				.pSubscriber		= &m_LocalPlayerEntities,
				.ComponentAccesses	=
				{
					{ NDA,	PlayerLocalComponent::Type() },
					{ RW,	PacketComponent<PacketPlayerAction>::Type() }
				},
				.ComponentGroups = { &playerGroup }
			},
		};

		systemReg.SubscriberRegistration.AdditionalAccesses = GetFireProjectileComponentAccesses();
		systemReg.Phase = 1;

		RegisterSystem(TYPE_NAME(WeaponSystemClient), systemReg);
	}

	// Create rendering resources for projectiles
	{
		MaterialProperties projectileMaterialProperties;
		projectileMaterialProperties.Albedo = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
		projectileMaterialProperties.Metallic = 0.5f;
		projectileMaterialProperties.Roughness = 0.5f;

		const uint32 projectileMeshGUID = ResourceManager::LoadMeshFromFile("sphere.obj");
		if (projectileMeshGUID == GUID_NONE)
		{
			return false;
		}

		// Paint
		m_RedPaintProjectileMeshComponent = { };
		m_RedPaintProjectileMeshComponent.MeshGUID		= projectileMeshGUID;
		m_RedPaintProjectileMeshComponent.MaterialGUID	= ResourceManager::LoadMaterialFromMemory(
			"Red Paint Projectile",
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_NORMAL_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			projectileMaterialProperties);

		projectileMaterialProperties.Albedo = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);

		m_BluePaintProjectileMeshComponent = { };
		m_BluePaintProjectileMeshComponent.MeshGUID		= projectileMeshGUID;
		m_BluePaintProjectileMeshComponent.MaterialGUID	= ResourceManager::LoadMaterialFromMemory(
			"Blue Paint Projectile",
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_NORMAL_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			projectileMaterialProperties);

		// Water
		projectileMaterialProperties.Albedo = glm::vec4(87.0f / 255.0f, 217.0f / 255.0f, 1.0f, 1.0f);

		m_WaterProjectileMeshComponent = { };
		m_WaterProjectileMeshComponent.MeshGUID		= projectileMeshGUID;
		m_WaterProjectileMeshComponent.MaterialGUID	= ResourceManager::LoadMaterialFromMemory(
			"Water Projectile",
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_NORMAL_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			projectileMaterialProperties);
	}

	// Create soundeffects
	m_GunFireGUID	= ResourceManager::LoadSoundEffect3DFromFile("gun.wav");
	if (m_GunFireGUID == GUID_NONE)
	{
		return false;
	}

	m_OutOfAmmoGUID = ResourceManager::LoadSoundEffect2DFromFile("out_of_ammo.wav");
	if (m_OutOfAmmoGUID == GUID_NONE)
	{
		return false;
	}

	return true;
}

bool WeaponSystemClient::TryFire(EAmmoType ammoType, LambdaEngine::Entity weaponEntity)
{
	using namespace LambdaEngine;

	const bool didFire = WeaponSystem::TryFire(ammoType, weaponEntity);
	if (didFire)
	{
		ECSCore* pECS = ECSCore::GetInstance();
		const WeaponComponent& weaponComponent			= pECS->GetConstComponent<WeaponComponent>(weaponEntity);
		PacketComponent<PacketPlayerAction>& packets	= pECS->GetComponent<PacketComponent<PacketPlayerAction>>(weaponComponent.WeaponOwner);
		
		// Send action to server
		TQueue<PacketPlayerAction>& actions = packets.GetPacketsToSend();
		if (!actions.empty())
		{
			actions.back().FiredAmmo = ammoType;
		}
	}
	else
	{
		// Play out of ammo
		ISoundEffect2D* pSound = ResourceManager::GetSoundEffect2D(m_OutOfAmmoGUID);
		pSound->PlayOnce();
	}

	return didFire;
}
