#include "ECS/Systems/Player/WeaponSystem.h"
#include "ECS/Components/Player/Player.h"
#include "ECS/ECSCore.h"

#include "Application/API/Events/EventQueue.h"

#include "Game/ECS/Systems/Physics/PhysicsSystem.h"

#include "Events/GameplayEvents.h"

#include "Input/API/Input.h"

#include "Physics/PhysicsEvents.h"

#include "Resources/Material.h"
#include "Resources/ResourceManager.h"

#include "Physics/CollisionGroups.h"

#include "Game/Multiplayer/MultiplayerUtils.h"

static const glm::vec3 PROJECTILE_OFFSET = glm::vec3(0.0f, 1.0f, 0.0f);

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
					{ RW, PacketComponent<PlayerActionResponse>::Type() }
				}
			},
			{
				.pSubscriber = &m_LocalPlayerEntities,
				.ComponentAccesses =
				{
					{ NDA, PlayerLocalComponent::Type() },
					{ RW, PacketComponent<PlayerAction>::Type() }
				},
				.ComponentGroups = { &playerGroup }
			},
			{
				.pSubscriber = &m_RemotePlayerEntities,
				.ComponentAccesses =
				{
					{ NDA, PlayerBaseComponent::Type() },
					{ R, PacketComponent<PlayerAction>::Type() },
					{ RW, PacketComponent<PlayerActionResponse>::Type() },
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
		m_PaintProjectileMeshComponent = { };
		m_PaintProjectileMeshComponent.MeshGUID		= projectileMeshGUID;
		m_PaintProjectileMeshComponent.MaterialGUID = ResourceManager::LoadMaterialFromMemory(
			"Paint Projectile",
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_NORMAL_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			projectileMaterialProperties);

		// Water
		projectileMaterialProperties.Albedo = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);

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
	m_GunFireGUID	= ResourceManager::LoadSoundEffectFromFile("Fart.wav");
	m_OutOfAmmoGUID	= ResourceManager::LoadSoundEffectFromFile("out_of_ammo.wav");
	return true;
}

void WeaponSystem::FixedTick(LambdaEngine::Timestamp deltaTime)
{
	using namespace LambdaEngine;
	const float32 dt = (float32)deltaTime.AsSeconds();

	ECSCore* pECS = ECSCore::GetInstance();
	ComponentArray<WeaponComponent>* pWeaponComponents = pECS->GetComponentArray<WeaponComponent>();
	ComponentArray<PacketComponent<PlayerAction>>* pPlayerActionPackets = pECS->GetComponentArray<PacketComponent<PlayerAction>>();
	ComponentArray<PacketComponent<PlayerActionResponse>>* pPlayerResponsePackets = pECS->GetComponentArray<PacketComponent<PlayerActionResponse>>();

	const ComponentArray<PositionComponent>* 	pPositionComponents = pECS->GetComponentArray<PositionComponent>();
	const ComponentArray<RotationComponent>* 	pRotationComponents = pECS->GetComponentArray<RotationComponent>();
	const ComponentArray<VelocityComponent>* 	pVelocityComponents = pECS->GetComponentArray<VelocityComponent>();
	const ComponentArray<OffsetComponent>* 		pOffsetComponents	= pECS->GetComponentArray<OffsetComponent>();

	if (MultiplayerUtils::IsServer())
	{
		// On server fire projectiles to clients
		for (Entity remoteEntity : m_RemotePlayerEntities)
		{
			PacketComponent<PlayerAction>&			actionsRecived	= pPlayerActionPackets->GetData(remoteEntity);
			PacketComponent<PlayerActionResponse>&	responsesToSend	= pPlayerResponsePackets->GetData(remoteEntity);

			TQueue<PlayerActionResponse>& packetsToSend = responsesToSend.GetPacketsToSend();
			const TArray<PlayerAction>& packetsRecived = actionsRecived.GetPacketsReceived();
			
			const uint32 packetCount = packetsRecived.GetSize();
			for (uint32 i = 0; i < packetCount; i++)
			{
				// TODO: Check on server if player has projectiles

				if (packetsRecived[i].FiredAmmo != EAmmoType::AMMO_TYPE_NONE)
				{
					EAmmoType ammoType = packetsRecived[i].FiredAmmo;
					packetsToSend.back().FiredAmmo = ammoType;

					const PositionComponent& positionComp = pPositionComponents->GetConstData(remoteEntity);
					const VelocityComponent& velocityComp = pVelocityComponents->GetConstData(remoteEntity);
					const RotationComponent& rotationComp = pRotationComponents->GetConstData(remoteEntity);

					LOG_INFO("Player=%d fired at(x=%.4f, y=%.4f, z=%.4f)", remoteEntity, positionComp.Position.x, positionComp.Position.y, positionComp.Position.z);
					Fire(ammoType, remoteEntity, positionComp.Position, rotationComp.Quaternion, velocityComp.Velocity);
				}
			}
		}
	}
	else
	{
		// On client fire projectiles from other players
		for (Entity foreignEntity : m_ForeignPlayerEntities)
		{
			PacketComponent<PlayerActionResponse>& packets = pPlayerResponsePackets->GetData(foreignEntity);
			const TArray<PlayerActionResponse>& receivedPackets = packets.GetPacketsReceived();
			for (const PlayerActionResponse& response : receivedPackets)
			{
				if (response.FiredAmmo != EAmmoType::AMMO_TYPE_NONE)
				{
					const glm::vec3 position = response.Position + PROJECTILE_OFFSET;

					LOG_INFO("Player=%d fired at(x=%.4f, y=%.4f, z=%.4f)", foreignEntity, position.x, position.y, position.z);
					Fire(response.FiredAmmo, foreignEntity, position, response.Rotation, response.Velocity);
				}
			}
		}

		// TODO: Check local response and maybe roll back

		// Then handle local projectiles
		for (Entity weaponEntity : m_WeaponEntities)
		{
			WeaponComponent& weaponComponent = pWeaponComponents->GetData(weaponEntity);
			Entity playerEntity = weaponComponent.WeaponOwner;
			if (!m_LocalPlayerEntities.HasElement(playerEntity))
			{
				continue;
			}

			PacketComponent<PlayerAction>& playerActions = pPlayerActionPackets->GetData(playerEntity);
			const bool hasAmmo		= weaponComponent.CurrentAmmunition > 0;
			const bool isReloading	= weaponComponent.ReloadClock > 0.0f;
			if (!hasAmmo && !isReloading)
			{
				StartReload(weaponComponent);
			}

			const bool onCooldown = weaponComponent.CurrentCooldown > 0.0f;
			if (onCooldown)
			{
				weaponComponent.CurrentCooldown -= dt;
			}

			if (isReloading)
			{
				LOG_INFO("Reloading");

				weaponComponent.ReloadClock -= dt;
				if (weaponComponent.ReloadClock < 0.0f)
				{
					weaponComponent.ReloadClock = 0.0f;
					weaponComponent.CurrentAmmunition = AMMO_CAPACITY;

					LOG_INFO("Reload Finish");
				}
			}

			// Update position and orientation of weapon component
			const PositionComponent& playerPositionComp = pPositionComponents->GetConstData(playerEntity);
			const RotationComponent& playerRotationComp = pRotationComponents->GetConstData(playerEntity);
			const OffsetComponent& weaponOffsetComp = pOffsetComponents->GetConstData(weaponEntity);
			glm::vec3 weaponPosition;
			if (playerRotationComp.Dirty || playerPositionComp.Dirty)
			{
				PositionComponent& weaponPositionComp = pPositionComponents->GetData(weaponEntity);
				RotationComponent& weaponRotationComp = pRotationComponents->GetData(weaponEntity);

				glm::quat quatY = playerRotationComp.Quaternion;
				quatY.x = 0;
				quatY.z = 0;
				quatY = glm::normalize(quatY);
				weaponPositionComp.Position = playerPositionComp.Position + quatY * weaponOffsetComp.Offset;
				weaponPosition = weaponPositionComp.Position;

				weaponRotationComp.Quaternion = playerRotationComp.Quaternion;
			}
			else
			{
				weaponPosition = pPositionComponents->GetConstData(weaponEntity).Position;
			}

			// Reload if we are not reloading
			if (Input::IsKeyDown(EKey::KEY_R) && !isReloading)
			{
				StartReload(weaponComponent);
			}
			else if (!onCooldown) // If we did not hit the reload try and shoot
			{
				const VelocityComponent& velocityComp = pVelocityComponents->GetConstData(playerEntity);
				const RotationComponent& rotationComp = pRotationComponents->GetConstData(playerEntity);

				const glm::vec3 firePosition = weaponPosition;
				if (Input::GetMouseState(InputMode::GAME).IsButtonPressed(EMouseButton::MOUSE_BUTTON_LEFT))
				{
					LOG_INFO("Fire At x=%.4f y=%.4f z=%.4f", firePosition.x, firePosition.y, firePosition.z);

					TryFire(
						EAmmoType::AMMO_TYPE_PAINT,
						weaponComponent,
						playerActions,
						weaponPosition,
						rotationComp.Quaternion,
						velocityComp.Velocity);
				}
				else if (Input::GetMouseState(InputMode::GAME).IsButtonPressed(EMouseButton::MOUSE_BUTTON_RIGHT))
				{
					TryFire(
						EAmmoType::AMMO_TYPE_WATER,
						weaponComponent,
						playerActions,
						weaponPosition,
						rotationComp.Quaternion,
						velocityComp.Velocity);
				}
			}
		}
	}
}

void WeaponSystem::Fire(
	EAmmoType ammoType,
	LambdaEngine::Entity weaponOwner,
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
	const glm::vec3 startPos = playerPos + g_DefaultUp + directionVec;

	ECSCore* pECS = ECSCore::GetInstance();
	const uint32	playerTeam		= pECS->GetConstComponent<TeamComponent>(weaponOwner).TeamIndex;
	const glm::vec3 initialVelocity = playerVelocity + directionVec * projectileInitialSpeed;

	LOG_INFO("[Fire]: At(x=%.4f, y=%.4f, z=%.4f) Velocity=(x=%.4f, y=%.4f, z=%.4f)", startPos.x, startPos.y, startPos.z, initialVelocity.x, initialVelocity.y, initialVelocity.z);

	const MeshComponent& meshComp = ammoType == EAmmoType::AMMO_TYPE_PAINT ? m_PaintProjectileMeshComponent : m_WaterProjectileMeshComponent;

	// Fire event
	WeaponFiredEvent firedEvent(
		weaponOwner,
		ammoType, 
		startPos,
		initialVelocity, 
		direction,
		playerTeam);
	firedEvent.Callback = std::bind_front(&WeaponSystem::OnProjectileHit, this);
	firedEvent.MeshComponent = meshComp;
	EventQueue::SendEventImmediate(firedEvent);

	// Play gun fire
	if (!MultiplayerUtils::IsServer())
	{
		ISoundEffect3D* m_pSound = ResourceManager::GetSoundEffect(m_GunFireGUID);
		m_pSound->PlayOnceAt(startPos, playerVelocity, 0.2f, 1.0f);
	}
}

void WeaponSystem::TryFire(
	EAmmoType ammoType,
	WeaponComponent& weaponComponent,
	PacketComponent<PlayerAction>& packets,
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
		TQueue<PlayerAction>& actions = packets.GetPacketsToSend();
		if (!actions.empty())
		{
			actions.back().FiredAmmo = ammoType;
		}

		// For creating entity
		Fire(ammoType, weaponComponent.WeaponOwner, startPos, direction, playerVelocity);
	}
	else
	{
		// Play out of ammo
		ISoundEffect3D* m_pSound = ResourceManager::GetSoundEffect(m_OutOfAmmoGUID);
		m_pSound->PlayOnceAt(startPos, playerVelocity, 1.0f, 1.0f);
	}
}

void WeaponSystem::TryFire(
	EAmmoType ammoType,
	WeaponComponent& weaponComponent,
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
		Fire(ammoType, weaponComponent.WeaponOwner, startPos, direction, playerVelocity);
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

	LOG_INFO("Projectile hit, collisionInfo0: %d, collisionInfo1: %d", collisionInfo0.Entity, collisionInfo1.Entity);

	// Is this safe? Concurrency issues?
	ECSCore* pECS = ECSCore::GetInstance();
	const ComponentArray<TeamComponent>*		pTeamComponents			= pECS->GetComponentArray<TeamComponent>();
	const ComponentArray<ProjectileComponent>*	pProjectileComponents	= pECS->GetComponentArray<ProjectileComponent>();

	pECS->RemoveEntity(collisionInfo0.Entity);

	// Disable friendly fire
	bool friendly = false;
	if (pTeamComponents->HasComponent(collisionInfo1.Entity))
	{
		const uint32 otherEntityTeam	= pTeamComponents->GetConstData(collisionInfo1.Entity).TeamIndex;
		const uint32 projectileTeam		= pTeamComponents->GetConstData(collisionInfo0.Entity).TeamIndex;

		if (projectileTeam == otherEntityTeam)
		{
			LOG_INFO("Friendly fire!");
			friendly = true;
		}
	}

	// Always destroy projectile but do not send event if we hit a friend
	pECS->RemoveEntity(collisionInfo0.Entity);
	if (!friendly)
	{
		EAmmoType ammoType = EAmmoType::AMMO_TYPE_NONE;
		if (pProjectileComponents->HasComponent(collisionInfo0.Entity))
		{
			ammoType = pProjectileComponents->GetConstData(collisionInfo0.Entity).AmmoType;
		}

		ProjectileHitEvent hitEvent(collisionInfo0, collisionInfo1, ammoType);
		EventQueue::SendEventImmediate(hitEvent);
	}
}

void WeaponSystem::StartReload(WeaponComponent& weaponComponent)
{
	LOG_INFO("Start reload");
	weaponComponent.ReloadClock = weaponComponent.ReloadTime;
}

void WeaponSystem::AbortReload(WeaponComponent& weaponComponent)
{
	LOG_INFO("Abort reload");
	weaponComponent.ReloadClock = 0;
}
