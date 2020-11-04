#include "ECS/Systems/Player/WeaponSystem.h"
#include "ECS/Components/Player/Player.h"
#include "ECS/ECSCore.h"

#include "Application/API/Events/EventQueue.h"

#include "Game/ECS/Systems/Physics/PhysicsSystem.h"
#include "Game/Multiplayer/MultiplayerUtils.h"
#include "Game/ECS/Components/Physics/Transform.h"

#include "Events/GameplayEvents.h"

// #include "Input/API/Input.h"
#include "Input/API/InputActionSystem.h"

#include "Physics/PhysicsEvents.h"
#include "Physics/CollisionGroups.h"

#include "Resources/Material.h"
#include "Resources/ResourceManager.h"

#include "Teams/TeamHelper.h"

/*
* Constants
*/

static const glm::vec3 PROJECTILE_OFFSET = glm::vec3(0.0f, 1.0f, 0.0f);

/*
* Helper
*/

static glm::vec3 CalculateWeaponPosition(
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

WeaponSystem WeaponSystem::s_Instance;

bool WeaponSystem::Init()
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
				.pSubscriber = &m_WeaponEntities,
				.ComponentAccesses =
				{
					{ RW, WeaponComponent::Type() },
				}
			},
			{
				.pSubscriber = &m_ForeignPlayerEntities,
				.ComponentAccesses =
				{
					{ NDA, PlayerForeignComponent::Type() },
					{ RW, PacketComponent<PacketPlayerActionResponse>::Type() }
				}
			},
			{
				.pSubscriber = &m_LocalPlayerEntities,
				.ComponentAccesses =
				{
					{ NDA, PlayerLocalComponent::Type() },
					{ RW, PacketComponent<PacketPlayerAction>::Type() }
				},
				.ComponentGroups = { &playerGroup }
			},
			{
				.pSubscriber = &m_RemotePlayerEntities,
				.ComponentAccesses =
				{
					{ NDA, PlayerBaseComponent::Type() },
					{ R, PacketComponent<PacketPlayerAction>::Type() },
					{ RW, PacketComponent<PacketPlayerActionResponse>::Type() },
				},
				.ComponentGroups = { &playerGroup }
			}
		};

		systemReg.SubscriberRegistration.AdditionalAccesses = GetFireProjectileComponentAccesses();
		systemReg.Phase = 1;

		RegisterSystem(TYPE_NAME(WeaponSystem), systemReg);
	}

	// Create rendering resources for projectiles
	{
		MaterialProperties projectileMaterialProperties;
		projectileMaterialProperties.Albedo		= glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
		projectileMaterialProperties.Metallic	= 0.5f;
		projectileMaterialProperties.Roughness	= 0.5f;

		const uint32 projectileMeshGUID = ResourceManager::LoadMeshFromFile("sphere.obj");

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
		m_WaterProjectileMeshComponent.MaterialGUID = ResourceManager::LoadMaterialFromMemory(
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
	m_OutOfAmmoGUID	= ResourceManager::LoadSoundEffectFromFile("out_of_ammo.wav");
	return true;
}

void WeaponSystem::FixedTick(LambdaEngine::Timestamp deltaTime)
{
	using namespace LambdaEngine;
	const float32 dt = (float32)deltaTime.AsSeconds();

	ECSCore* pECS = ECSCore::GetInstance();
	ComponentArray<PacketComponent<PacketPlayerAction>>* pPlayerActionPackets = pECS->GetComponentArray<PacketComponent<PacketPlayerAction>>();
	ComponentArray<PacketComponent<PacketPlayerActionResponse>>* pPlayerResponsePackets = pECS->GetComponentArray<PacketComponent<PacketPlayerActionResponse>>();
	ComponentArray<WeaponComponent>* pWeaponComponents = pECS->GetComponentArray<WeaponComponent>();
	ComponentArray<PositionComponent>* pPositionComponents = pECS->GetComponentArray<PositionComponent>();
	ComponentArray<RotationComponent>* pRotationComponents = pECS->GetComponentArray<RotationComponent>();
	const ComponentArray<VelocityComponent>* pVelocityComponents = pECS->GetComponentArray<VelocityComponent>();
	const ComponentArray<OffsetComponent>* pOffsetComponents = pECS->GetComponentArray<OffsetComponent>();

	// Handle projectiles
	if (MultiplayerUtils::IsServer())
	{
		for (Entity weaponEntity : m_WeaponEntities)
		{
			WeaponComponent& weaponComp = pWeaponComponents->GetData(weaponEntity);
			Entity remotePlayerEntity	= weaponComp.WeaponOwner;

			// Update weapon
			const bool hasAmmo		= weaponComp.CurrentAmmunition > 0;
			const bool isReloading	= weaponComp.ReloadClock > 0.0f;
			const bool onCooldown	= weaponComp.CurrentCooldown > 0.0f;

			if (onCooldown)
			{
				weaponComp.CurrentCooldown -= dt;
			}

			if (isReloading)
			{
				weaponComp.ReloadClock -= dt;
				if (weaponComp.ReloadClock < 0.0f)
				{
					weaponComp.ReloadClock = 0.0f;
					weaponComp.CurrentAmmunition = AMMO_CAPACITY;
				}
			}

			PacketComponent<PacketPlayerAction>& actionsRecived = pPlayerActionPackets->GetData(remotePlayerEntity);
			PacketComponent<PacketPlayerActionResponse>& responsesToSend = pPlayerResponsePackets->GetData(remotePlayerEntity);
			TQueue<PacketPlayerActionResponse>& packetsToSend	= responsesToSend.GetPacketsToSend();
			const TArray<PacketPlayerAction>& packetsRecived	= actionsRecived.GetPacketsReceived();

			// Handle packets
			const uint32 packetCount = packetsRecived.GetSize();
			for (uint32 i = 0; i < packetCount; i++)
			{
				// Start reload
				if (packetsRecived[i].StartedReload && !isReloading)
				{
					LOG_INFO("SERVER: STARTED RELOAD");
					weaponComp.ReloadClock = weaponComp.ReloadTime;
				}
				else if (packetsRecived[i].FiredAmmo != EAmmoType::AMMO_TYPE_NONE && !onCooldown)
				{
					// Only send if the weapon has ammo on the server
					if (hasAmmo)
					{
						// Update position and orientation of weapon component
						const PositionComponent& playerPositionComp = pPositionComponents->GetConstData(remotePlayerEntity);
						const RotationComponent& playerRotationComp = pRotationComponents->GetConstData(remotePlayerEntity);
						const VelocityComponent& velocityComp	= pVelocityComponents->GetConstData(remotePlayerEntity);
						const OffsetComponent& weaponOffsetComp	= pOffsetComponents->GetConstData(weaponEntity);
						PositionComponent& weaponPositionComp	= pPositionComponents->GetData(weaponEntity);
						RotationComponent& weaponRotationComp	= pRotationComponents->GetData(weaponEntity);
						glm::vec3 weaponPosition = CalculateWeaponPosition(
							playerPositionComp.Position,
							playerRotationComp.Quaternion,
							weaponPositionComp,
							weaponRotationComp,
							weaponOffsetComp);

						const EAmmoType ammoType = packetsRecived[i].FiredAmmo;
						packetsToSend.back().FiredAmmo = ammoType;

						// Handle fire
						weaponComp.CurrentAmmunition--;
						weaponComp.CurrentCooldown = 1.0f / weaponComp.FireRate;

						// Create projectile
						Fire(
							ammoType, 
							remotePlayerEntity,
							weaponEntity,
							weaponPosition,
							playerRotationComp.Quaternion, 
							velocityComp.Velocity);
					}
				}
			}
		}
	}
	else
	{
		// TODO: Check local response and maybe roll back

		// Then handle local projectiles
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
					weaponComponent.ReloadClock			= 0.0f;
					weaponComponent.CurrentAmmunition	= AMMO_CAPACITY;
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
}

void WeaponSystem::Fire(
	EAmmoType ammoType,
	LambdaEngine::Entity weaponOwner,
	LambdaEngine::Entity weaponEntity,
	const glm::vec3& playerPos,
	const glm::quat& direction,
	const glm::vec3& playerVelocity)
{
	using namespace LambdaEngine;

	if (ammoType == EAmmoType::AMMO_TYPE_NONE)
	{
		return;
	}

	constexpr const float projectileInitialSpeed = 13.0f;
	const glm::vec3 directionVec = GetForward(glm::normalize(direction));
	const glm::vec3 startPos = playerPos;

	ECSCore* pECS = ECSCore::GetInstance();
	const uint32	playerTeam = pECS->GetConstComponent<TeamComponent>(weaponOwner).TeamIndex;
	const glm::vec3 initialVelocity = playerVelocity + directionVec * projectileInitialSpeed;

	//LOG_INFO("[Fire]: At(x=%.4f, y=%.4f, z=%.4f) Velocity=(x=%.4f, y=%.4f, z=%.4f)", startPos.x, startPos.y, startPos.z, initialVelocity.x, initialVelocity.y, initialVelocity.z);

	MeshComponent* pMeshComp = nullptr;
	if (ammoType == EAmmoType::AMMO_TYPE_PAINT)
	{
		if (playerTeam == 0)
		{
			pMeshComp = &m_BluePaintProjectileMeshComponent;
		}
		else
		{
			pMeshComp = &m_RedPaintProjectileMeshComponent;
		}
	}
	else
	{
		pMeshComp = &m_WaterProjectileMeshComponent;
	}

	// Fire event
	WeaponFiredEvent firedEvent(
		weaponOwner,
		ammoType,
		startPos,
		initialVelocity,
		direction,
		playerTeam);
	firedEvent.Callback = std::bind_front(&WeaponSystem::OnProjectileHit, this);
	firedEvent.MeshComponent = *pMeshComp;
	EventQueue::SendEventImmediate(firedEvent);

	// Play gun fire and spawn particles
	if (!MultiplayerUtils::IsServer())
	{
		ISoundEffect3D* m_pSound = ResourceManager::GetSoundEffect(m_GunFireGUID);
		m_pSound->PlayOnceAt(startPos, playerVelocity, 0.2f, 1.0f);

		ParticleEmitterComponent& emitterComp = pECS->GetComponent<ParticleEmitterComponent>(weaponEntity);
		emitterComp.Active = true;
	}
}

void WeaponSystem::TryFire(
	EAmmoType ammoType,
	WeaponComponent& weaponComponent,
	PacketComponent<PacketPlayerAction>& packets,
	LambdaEngine::Entity weaponEntity,
	const glm::vec3& startPos,
	const glm::quat& direction,
	const glm::vec3& playerVelocity)
{
	using namespace LambdaEngine;

	// Add cooldown
	weaponComponent.CurrentCooldown = 1.0f / weaponComponent.FireRate;

	const bool hasAmmo = weaponComponent.CurrentAmmunition > 0;
	if (hasAmmo)
	{
		// If we try to shoot when reloading we abort the reload
		const bool isReloading = weaponComponent.ReloadClock > 0.0f;
		if (isReloading)
		{
			AbortReload(weaponComponent);
		}

		// Fire the gun
		weaponComponent.CurrentAmmunition--;

		// Send action to server
		TQueue<PacketPlayerAction>& actions = packets.GetPacketsToSend();
		if (!actions.empty())
		{
			actions.back().FiredAmmo = ammoType;
		}

		// For creating entity
		Fire(
			ammoType, 
			weaponComponent.WeaponOwner, 
			weaponEntity, 
			startPos, 
			direction, 
			playerVelocity);
	}
	else
	{
		// Play out of ammo
		ISoundEffect3D* m_pSound = ResourceManager::GetSoundEffect(m_OutOfAmmoGUID);
		m_pSound->PlayOnceAt(startPos, playerVelocity, 1.0f, 1.0f);
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
			//LOG_INFO("Friendly fire!");
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
