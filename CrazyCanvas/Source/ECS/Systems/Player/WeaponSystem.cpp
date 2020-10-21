#include "ECS/Systems/Player/WeaponSystem.h"
#include "ECS/Components/Player/Player.h"
#include "ECS/Components/Player/WeaponComponent.h"
#include "ECS/Components/Team/TeamComponent.h"
#include "ECS/ECSCore.h"

#include "Application/API/Events/EventQueue.h"

#include "Game/ECS/Systems/Physics/PhysicsSystem.h"

#include "Input/API/Input.h"

#include "Physics/PhysicsEvents.h"

#include "Resources/Material.h"
#include "Resources/ResourceManager.h"

bool WeaponSystem::Init()
{
	using namespace LambdaEngine;

	// Register system
	{
		// The write permissions are used when creating projectile entities
		PlayerGroup playerGroup;
		playerGroup.Position.Permissions = RW;
		playerGroup.Scale.Permissions = RW;
		playerGroup.Rotation.Permissions = RW;
		playerGroup.Velocity.Permissions = RW;

		SystemRegistration systemReg = {};
		systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
		{
			{
				.pSubscriber = &m_WeaponEntities,
				.ComponentAccesses =
				{
					{RW, WeaponComponent::Type()}
				}
			},
			{
				.pSubscriber = &m_PlayerEntities,
				.ComponentAccesses =
				{
					{NDA, PlayerLocalComponent::Type()}
				},
				.ComponentGroups = { &playerGroup }
			}
		};
		systemReg.SubscriberRegistration.AdditionalAccesses =
		{
			{RW, DynamicCollisionComponent::Type()}, {RW, MeshComponent::Type()}, {RW, TeamComponent::Type()}
		};
		systemReg.Phase = 1;

		RegisterSystem(systemReg);
	}

	// Create rendering resources for projectiles
	{
		MaterialProperties projectileMaterialProperties;
		projectileMaterialProperties.Albedo		= glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
		projectileMaterialProperties.Metallic	= 0.5f;
		projectileMaterialProperties.Roughness	= 0.5f;

		const uint32 projectileMeshGUID = ResourceManager::LoadMeshFromFile("sphere.obj");

		// Paint
		m_PaintProjectileMeshComponent = {};
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

		m_WaterProjectileMeshComponent = {};
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
	m_GunFireGUID	= ResourceManager::LoadSoundEffectFromFile("9_mm_gunshot-mike-koenig-123.wav");
	m_OutOfAmmoGUID	= ResourceManager::LoadSoundEffectFromFile("out_of_ammo.wav");
	return true;
}

void WeaponSystem::Tick(LambdaEngine::Timestamp deltaTime)
{
	using namespace LambdaEngine;
	const float32 dt = (float32)deltaTime.AsSeconds();

	ECSCore* pECS = ECSCore::GetInstance();
	ComponentArray<WeaponComponent>* pWeaponComponents = pECS->GetComponentArray<WeaponComponent>();
	const ComponentArray<PositionComponent>* pPositionComponents = pECS->GetComponentArray<PositionComponent>();
	const ComponentArray<RotationComponent>* pRotationComponents = pECS->GetComponentArray<RotationComponent>();
	const ComponentArray<VelocityComponent>* pVelocityComponents = pECS->GetComponentArray<VelocityComponent>();

	for (Entity weaponEntity : m_WeaponEntities)
	{
		WeaponComponent& weaponComponent = pWeaponComponents->GetData(weaponEntity);
		Entity playerEntity = weaponComponent.WeaponOwner;
		if (!m_PlayerEntities.HasElement(playerEntity))
		{
			continue;
		}

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
				weaponComponent.ReloadClock			= 0.0f;
				weaponComponent.CurrentAmmunition	= 5;

				LOG_INFO("Reload Finish");
			}
		}

		// Reload if we are not reloading
		if (Input::IsKeyDown(EKey::KEY_R) && !isReloading)
		{
			StartReload(weaponComponent);
		}
		else if (!onCooldown) // If we did not hit the reload try and shoot
		{
			if (Input::GetMouseState().IsButtonPressed(EMouseButton::MOUSE_BUTTON_FORWARD))
			{
				const PositionComponent& positionComp = pPositionComponents->GetConstData(playerEntity);
				const VelocityComponent& velocityComp = pVelocityComponents->GetConstData(playerEntity);
				const RotationComponent& rotationComp = pRotationComponents->GetConstData(playerEntity);

				TryFire(EAmmoType::AMMO_TYPE_PAINT, weaponComponent, positionComp.Position + glm::vec3(0.0f, 1.0f, 0.0f), rotationComp.Quaternion, velocityComp.Velocity);
			}
			else if (Input::GetMouseState().IsButtonPressed(EMouseButton::MOUSE_BUTTON_BACK))
			{
				const PositionComponent& positionComp = pPositionComponents->GetConstData(playerEntity);
				const VelocityComponent& velocityComp = pVelocityComponents->GetConstData(playerEntity);
				const RotationComponent& rotationComp = pRotationComponents->GetConstData(playerEntity);

				TryFire(EAmmoType::AMMO_TYPE_WATER, weaponComponent, positionComp.Position + glm::vec3(0.0f, 1.0f, 0.0f), rotationComp.Quaternion, velocityComp.Velocity);
			}
		}
	}
}

void WeaponSystem::Fire(EAmmoType ammoType, WeaponComponent& weaponComponent, const glm::vec3& playerPos, const glm::quat& direction, const glm::vec3& playerVelocity)
{
	using namespace LambdaEngine;

	// Tick down ammunition
	weaponComponent.CurrentAmmunition--;

	constexpr const float projectileInitialSpeed = 13.0f;
	const glm::vec3 directionVec = GetForward(direction);
	const glm::vec3 startPos = playerPos + g_DefaultUp + directionVec * 0.3f;

	// Create a projectile entity
	ECSCore* pECS = ECSCore::GetInstance();
	const Entity projectileEntity = pECS->CreateEntity();

	// Get the firing player's team index
	const uint32 playerTeam = pECS->GetConstComponent<TeamComponent>(weaponComponent.WeaponOwner).TeamIndex;
	pECS->AddComponent<TeamComponent>(projectileEntity, { playerTeam });

	const VelocityComponent initialVelocity = { playerVelocity + directionVec * projectileInitialSpeed };
	pECS->AddComponent<VelocityComponent>(projectileEntity, initialVelocity);

	const ProjectileComponent projectileInfo = { ammoType };
	pECS->AddComponent<ProjectileComponent>(projectileEntity, projectileInfo);

	const MeshComponent& meshComp = ammoType == EAmmoType::AMMO_TYPE_PAINT ? m_PaintProjectileMeshComponent : m_WaterProjectileMeshComponent;
	const DynamicCollisionCreateInfo collisionInfo = 
	{
		/* Entity */	 		projectileEntity,
		/* Position */	 		pECS->AddComponent<PositionComponent>(projectileEntity, {true, startPos}),
		/* Scale */				pECS->AddComponent<ScaleComponent>(projectileEntity, {true, { 0.3f, 0.3f, 0.3f }}),
		/* Rotation */			pECS->AddComponent<RotationComponent>(projectileEntity, {true, direction}),
		/* Mesh */				pECS->AddComponent<MeshComponent>(projectileEntity, {meshComp}),
		/* Shape Type */		EShapeType::SIMULATION,
		/* CollisionGroup */	FCollisionGroup::COLLISION_GROUP_DYNAMIC,
		/* CollisionMask */		FCollisionGroup::COLLISION_GROUP_PLAYER | FCollisionGroup::COLLISION_GROUP_STATIC,
		/* CallbackFunction */	std::bind_front(&WeaponSystem::OnProjectileHit, this),
		/* Velocity */			initialVelocity
	};

	const DynamicCollisionComponent projectileCollisionComp = PhysicsSystem::GetInstance()->CreateDynamicCollisionSphere(collisionInfo);
	pECS->AddComponent<DynamicCollisionComponent>(projectileEntity, projectileCollisionComp);

	// Play gun fire
	ISoundEffect3D* m_pSound = ResourceManager::GetSoundEffect(m_GunFireGUID);
	m_pSound->PlayOnceAt(startPos, playerVelocity, 1.0f, 1.0f);
}

void WeaponSystem::OnProjectileHit(const LambdaEngine::EntityCollisionInfo& collisionInfo0, const LambdaEngine::EntityCollisionInfo& collisionInfo1)
{
	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();

	// Is this safe? Concurrency issues?
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

void WeaponSystem::TryFire(EAmmoType ammoType, WeaponComponent& weaponComponent, const glm::vec3& startPos, const glm::quat& direction, const glm::vec3& playerVelocity)
{
	using namespace LambdaEngine;

	// Add cooldown
	weaponComponent.CurrentCooldown = 1.0f / weaponComponent.FireRate;

	const bool hasAmmo = weaponComponent.CurrentAmmunition > 0;
	if (hasAmmo)
	{
		LOG_INFO("Fire paint");

		// If we try to shoot when reloading we abort the reload
		const bool isReloading = weaponComponent.ReloadClock > 0.0f;
		if (isReloading)
		{
			AbortReload(weaponComponent);
		}

		// Fire the gun
		Fire(ammoType, weaponComponent, startPos + glm::vec3(0.0f, 1.0f, 0.0f), direction, playerVelocity);
	}
	else
	{
		// Play out of ammo
		ISoundEffect3D* m_pSound = ResourceManager::GetSoundEffect(m_OutOfAmmoGUID);
		m_pSound->PlayOnceAt(startPos, playerVelocity, 1.0f, 1.0f);
	}
}
