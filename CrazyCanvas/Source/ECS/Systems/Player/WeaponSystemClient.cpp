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
	ComponentArray<WeaponComponent>*	pWeaponComponents	= pECS->GetComponentArray<WeaponComponent>();
	ComponentArray<PositionComponent>*	pPositionComponents	= pECS->GetComponentArray<PositionComponent>();
	ComponentArray<RotationComponent>*	pRotationComponents	= pECS->GetComponentArray<RotationComponent>();
	const ComponentArray<VelocityComponent>*	pVelocityComponents	= pECS->GetComponentArray<VelocityComponent>();
	const ComponentArray<OffsetComponent>*		pOffsetComponents	= pECS->GetComponentArray<OffsetComponent>();

	// TODO: Check local response and maybe roll back
	for (Entity weaponEntity : m_WeaponEntities)
	{
		WeaponComponent& weaponComponent = pWeaponComponents->GetData(weaponEntity);
		Entity playerEntity = weaponComponent.WeaponOwner;

		// Foreign Players
		if (!m_LocalPlayerEntities.HasElement(playerEntity))
		{
			PacketComponent<PacketPlayerActionResponse>& packets = pPlayerResponsePackets->GetData(playerEntity);
			const TArray<PacketPlayerActionResponse>& receivedPackets = packets.GetPacketsReceived();
			for (const PacketPlayerActionResponse& response : receivedPackets)
			{
				if (response.FiredAmmo != EAmmoType::AMMO_TYPE_NONE)
				{
					const OffsetComponent& weaponOffsetComp = pOffsetComponents->GetConstData(weaponEntity);
					PositionComponent& weaponPositionComp = pPositionComponents->GetData(weaponEntity);
					RotationComponent& weaponRotationComp = pRotationComponents->GetData(weaponEntity);
					glm::vec3 weaponPosition = CalculateWeaponPosition(
						response.Position,
						response.Rotation,
						weaponPositionComp,
						weaponRotationComp,
						weaponOffsetComp);

					Fire(
						response.FiredAmmo,
						playerEntity,
						weaponEntity,
						weaponPosition,
						response.Rotation,
						response.Velocity);
				}
			}

			continue;
		}

		// LocalPlayers
		PacketComponent<PacketPlayerAction>& playerActions = pPlayerActionPackets->GetData(playerEntity);
		const bool hasAmmo		= weaponComponent.CurrentAmmunition > 0;
		const bool isReloading	= weaponComponent.ReloadClock > 0.0f;
		if (!hasAmmo && !isReloading)
		{
			StartReload(weaponComponent, playerActions);
		}

		const bool onCooldown = weaponComponent.CurrentCooldown > 0.0f;
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
				weaponComponent.CurrentAmmunition = AMMO_CAPACITY;
			}
		}

		// Update position and orientation of weapon component
		const PositionComponent& playerPositionComp = pPositionComponents->GetConstData(playerEntity);
		const RotationComponent& playerRotationComp = pRotationComponents->GetConstData(playerEntity);
		const OffsetComponent& weaponOffsetComp	= pOffsetComponents->GetConstData(weaponEntity);
		PositionComponent& weaponPositionComp	= pPositionComponents->GetData(weaponEntity);
		RotationComponent& weaponRotationComp	= pRotationComponents->GetData(weaponEntity);
		glm::vec3 weaponPosition = CalculateWeaponPosition(
			playerPositionComp.Position,
			playerRotationComp.Quaternion,
			weaponPositionComp,
			weaponRotationComp,
			weaponOffsetComp);

		// Reload if we are not reloading
		if (InputActionSystem::IsActive(EAction::ACTION_ATTACK_RELOAD) && !isReloading)
		{
			StartReload(weaponComponent, playerActions);
		}
		else if (!onCooldown) // If we did not hit the reload try and shoot
		{
			const VelocityComponent& velocityComp = pVelocityComponents->GetConstData(playerEntity);
			if (InputActionSystem::IsActive(EAction::ACTION_ATTACK_PRIMARY))
			{
				TryFire(
					EAmmoType::AMMO_TYPE_PAINT,
					weaponComponent,
					playerActions,
					weaponEntity,
					weaponPosition,
					playerRotationComp.Quaternion,
					velocityComp.Velocity);
			}
			else if (InputActionSystem::IsActive(EAction::ACTION_ATTACK_SECONDARY))
			{
				TryFire(
					EAmmoType::AMMO_TYPE_WATER,
					weaponComponent,
					playerActions,
					weaponEntity,
					weaponPosition,
					playerRotationComp.Quaternion,
					velocityComp.Velocity);
			}
		}
	}
}

void WeaponSystemClient::Fire(
	EAmmoType ammoType, 
	LambdaEngine::Entity weaponOwner, 
	LambdaEngine::Entity weaponEntity, 
	const glm::vec3& playerPos, 
	const glm::quat& direction, 
	const glm::vec3& playerVelocity)
{
	using namespace LambdaEngine;

	WeaponSystem::Fire(
		ammoType,
		weaponOwner,
		weaponEntity,
		playerPos,
		direction,
		playerVelocity);

	ECSCore* pECS = ECSCore::GetInstance();

	// Play gun fire and spawn particles
	ISoundEffect3D* m_pSound = ResourceManager::GetSoundEffect(m_GunFireGUID);
	m_pSound->PlayOnceAt(playerPos, playerVelocity, 0.2f, 1.0f);

	ParticleEmitterComponent& emitterComp = pECS->GetComponent<ParticleEmitterComponent>(weaponEntity);
	emitterComp.Active = true;
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
				}
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
	m_GunFireGUID	= ResourceManager::LoadSoundEffectFromFile("gun.wav");
	if (m_GunFireGUID == GUID_NONE)
	{
		return false;
	}

	m_OutOfAmmoGUID = ResourceManager::LoadSoundEffectFromFile("out_of_ammo.wav");
	if (m_OutOfAmmoGUID == GUID_NONE)
	{
		return false;
	}

	return true;
}

bool WeaponSystemClient::TryFire(
	EAmmoType ammoType,
	WeaponComponent& weaponComponent,
	PacketComponent<PacketPlayerAction>& packets,
	LambdaEngine::Entity weaponEntity,
	const glm::vec3& startPos,
	const glm::quat& direction,
	const glm::vec3& playerVelocity)
{
	using namespace LambdaEngine;

	const bool didFire = WeaponSystem::TryFire(
		ammoType,
		weaponComponent,
		packets,
		weaponEntity,
		startPos,
		direction,
		playerVelocity);

	if (!didFire)
	{
		// Play out of ammo
		ISoundEffect3D* m_pSound = ResourceManager::GetSoundEffect(m_OutOfAmmoGUID);
		m_pSound->PlayOnceAt(startPos, playerVelocity, 1.0f, 1.0f);
	}

	return didFire;
}

