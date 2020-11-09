#include "ECS/Systems/Player/WeaponSystemClient.h"
#include "ECS/Components/Player/Player.h"
#include "ECS/ECSCore.h"

#include "Resources/Material.h"
#include "Resources/ResourceManager.h"

#include "Input/API/InputActionSystem.h"

/*
* WeaponSystemClients
*/

void WeaponSystemClient::Tick(LambdaEngine::Timestamp deltaTime)
{
	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();

	const ComponentArray<WeaponComponent>*		pWeaponComponents				= pECS->GetComponentArray<WeaponComponent>();
	ComponentArray<PositionComponent>*			pPositionComponents				= pECS->GetComponentArray<PositionComponent>();
	ComponentArray<RotationComponent>*			pRotationComponents				= pECS->GetComponentArray<RotationComponent>();
	ComponentArray<ScaleComponent>*				pScaleComponent					= pECS->GetComponentArray<ScaleComponent>();
	ComponentArray<AnimationAttachedComponent>* pAnimationAttachedComponents	= pECS->GetComponentArray<AnimationAttachedComponent>();

	for (Entity weaponEntity : m_WeaponEntities)
	{
		const WeaponComponent&				weaponComponent				= pWeaponComponents->GetConstData(weaponEntity);
		const AnimationAttachedComponent&	animationAttachedComponent	= pAnimationAttachedComponents->GetConstData(weaponEntity);

		PositionComponent&					weaponPositionComponent		= pPositionComponents->GetData(weaponEntity);
		RotationComponent&					weaponRotationComponent		= pRotationComponents->GetData(weaponEntity);
		ScaleComponent&						weaponScaleComponent		= pScaleComponent->GetData(weaponEntity);

		const PositionComponent&			playerPositionComponent		= pPositionComponents->GetConstData(weaponComponent.WeaponOwner);
		const RotationComponent&			playerRotationComponent		= pRotationComponents->GetConstData(weaponComponent.WeaponOwner);
		const ScaleComponent&				playerScaleComponent		= pScaleComponent->GetConstData(weaponComponent.WeaponOwner);

		glm::mat4 transform = glm::translate(playerPositionComponent.Position);

		glm::quat playerRotation = playerRotationComponent.Quaternion;
		playerRotation.x = 0;
		playerRotation.z = 0;
		playerRotation = glm::normalize(playerRotation);

		transform = transform * glm::toMat4(playerRotation);
		transform = glm::scale(transform, playerScaleComponent.Scale);

		transform = transform * animationAttachedComponent.Transform;

		glm::vec3 newScale;
		glm::quat newRotation;
		glm::vec3 newTranslation;
		glm::vec3 newSkew;
		glm::vec4 newPerspective;
		glm::decompose(
			transform,
			newScale,
			newRotation,
			newTranslation,
			newSkew,
			newPerspective);

		weaponPositionComponent.Position	= newTranslation;
		weaponRotationComponent.Quaternion	= newRotation;
		weaponScaleComponent.Scale			= newScale;
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
		if (!m_LocalPlayerEntities.HasElement(playerEntity))
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
						teamComponent.TeamIndex);
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

void WeaponSystemClient::Fire(LambdaEngine::Entity weaponEntity, WeaponComponent& weaponComponent, EAmmoType ammoType, const glm::vec3& position, const glm::vec3& velocity, uint32 playerTeam)
{
	using namespace LambdaEngine;

	WeaponSystem::Fire(weaponEntity, weaponComponent, ammoType, position, velocity, playerTeam);

	// Play gun fire and spawn particles
	ISoundEffect3D* m_pSound = ResourceManager::GetSoundEffect3D(m_GunFireGUID);
	m_pSound->PlayOnceAt(position, velocity, 0.2f, 1.0f);
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

	// Create rendering resources for projectiles
	{
		MaterialProperties projectileMaterialProperties;
		projectileMaterialProperties.Albedo = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
		projectileMaterialProperties.Metallic = 0.5f;
		projectileMaterialProperties.Roughness = 0.5f;

		GUID_Lambda projectileMeshGUID;
		GUID_Lambda projectileMaterialGUID;
		ResourceManager::LoadMeshFromFile("sphere.obj", projectileMeshGUID, projectileMaterialGUID);
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

		//Calculate Weapon Fire Properties (Position, Velocity and Team)
		glm::vec3 firePosition;
		glm::vec3 fireVelocity;
		uint32 playerTeam;
		CalculateWeaponFireProperties(weaponEntity, firePosition, fireVelocity, playerTeam);

		// For creating entity
		Fire(weaponEntity, weaponComponent, ammoType, firePosition, fireVelocity, playerTeam);

		// Send action to server
		PacketComponent<PacketPlayerAction>& packets = pECS->GetComponent<PacketComponent<PacketPlayerAction>>(weaponComponent.WeaponOwner);
		TQueue<PacketPlayerAction>& actions = packets.GetPacketsToSend();
		if (!actions.empty())
		{
			actions.back().FiredAmmo = ammoType;
		}

		return true;
	}
	else
	{
		// Play out of ammo
		ISoundEffect2D* pSound = ResourceManager::GetSoundEffect2D(m_OutOfAmmoGUID);
		pSound->PlayOnce(1.0f, 1.0f);

		return false;
	}
}

