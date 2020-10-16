#include "ECS/Systems/Player/WeaponSystem.h"

#include "Application/API/Events/EventQueue.h"
#include "ECS/Components/Player/Player.h"
#include "ECS/Components/Player/Weapon.h"
#include "ECS/Components/Team/TeamComponent.h"
#include "ECS/ECSCore.h"
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
		PlayerGroup playerGroup;
		playerGroup.Position.Permissions = R;
		playerGroup.Rotation.Permissions = R;

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

		m_ProjectileMeshComponent = {};
		m_ProjectileMeshComponent.MeshGUID = projectileMeshGUID;
		m_ProjectileMeshComponent.MaterialGUID = ResourceManager::LoadMaterialFromMemory(
			"Weapon Projectile",
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_NORMAL_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			GUID_TEXTURE_DEFAULT_COLOR_MAP,
			projectileMaterialProperties);
	}

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

		const int onCooldown = weaponComponent.CurrentCooldown > 0.0f;
		weaponComponent.CurrentCooldown -= onCooldown * dt;

		if (Input::GetMouseState().IsButtonPressed(EMouseButton::MOUSE_BUTTON_FORWARD) && !onCooldown)
		{
			const PositionComponent& positionComp = pPositionComponents->GetConstData(playerEntity);
			const RotationComponent& rotationComp = pRotationComponents->GetConstData(playerEntity);
			const VelocityComponent& velocityComp = pVelocityComponents->GetConstData(playerEntity);

			Fire(weaponComponent, positionComp.Position + glm::vec3(0.0f, 1.0f, 0.0f), rotationComp.Quaternion, velocityComp.Velocity);
		}
	}
}

void WeaponSystem::Fire(WeaponComponent& weaponComponent, const glm::vec3& startPos, const glm::quat& direction, const glm::vec3& playerVelocity)
{
	using namespace LambdaEngine;

	weaponComponent.CurrentCooldown = 1.0f / weaponComponent.FireRate;

	constexpr const float projectileInitialSpeed = 13.0f;
	const glm::vec3 directionVec = GetForward(direction);

	// Create a projectile entity
	ECSCore* pECS = ECSCore::GetInstance();
	const Entity projectileEntity = pECS->CreateEntity();

	// Get the firing player's team index
	const uint32 playerTeam = pECS->GetConstComponent<TeamComponent>(weaponComponent.WeaponOwner).TeamIndex;
	pECS->AddComponent<TeamComponent>(projectileEntity, { playerTeam });

	const VelocityComponent initialVelocity = {playerVelocity + directionVec * projectileInitialSpeed};
	pECS->AddComponent<VelocityComponent>(projectileEntity, initialVelocity);

	const DynamicCollisionCreateInfo collisionInfo = {
		/* Entity */	 		projectileEntity,
		/* Position */	 		pECS->AddComponent<PositionComponent>(projectileEntity, {true, startPos}),
		/* Scale */				pECS->AddComponent<ScaleComponent>(projectileEntity, {true, { 0.3f, 0.3f, 0.3f }}),
		/* Rotation */			pECS->AddComponent<RotationComponent>(projectileEntity, {true, direction}),
		/* Mesh */				pECS->AddComponent<MeshComponent>(projectileEntity, {m_ProjectileMeshComponent}),
		/* CollisionGroup */	FCollisionGroup::COLLISION_GROUP_DYNAMIC,
		/* CollisionMask */		FCollisionGroup::COLLISION_GROUP_PLAYER | FCollisionGroup::COLLISION_GROUP_STATIC,
		/* CollisionCallback */ std::bind(&WeaponSystem::OnProjectileHit, this, std::placeholders::_1, std::placeholders::_2),
		/* Velocity */			initialVelocity
	};

	const DynamicCollisionComponent projectileCollisionComp = PhysicsSystem::GetInstance()->CreateDynamicCollisionSphere(collisionInfo);
	pECS->AddComponent<DynamicCollisionComponent>(projectileEntity, projectileCollisionComp);
}

void WeaponSystem::OnProjectileHit(const LambdaEngine::EntityCollisionInfo& collisionInfo0, const LambdaEngine::EntityCollisionInfo& collisionInfo1)
{
	using namespace LambdaEngine;

	LOG_INFO("Projectile hit, entity: %d", collisionInfo0.Entity);
	ECSCore* pECS = ECSCore::GetInstance();

	const ComponentArray<TeamComponent>* pTeamComponents = pECS->GetComponentArray<TeamComponent>();

	// Disable friendly fire
	if (pTeamComponents->HasComponent(collisionInfo1.Entity))
	{
		const uint32 otherEntityTeam = pTeamComponents->GetConstData(collisionInfo1.Entity).TeamIndex;
		const uint32 projectileTeam = pTeamComponents->GetConstData(collisionInfo0.Entity).TeamIndex;

		if (projectileTeam == otherEntityTeam)
		{
			LOG_INFO("Friendly fire!");
			return;
		}
	}

	pECS->RemoveEntity(collisionInfo0.Entity);

	ProjectileHitEvent hitEvent(collisionInfo0, collisionInfo1);
	EventQueue::SendEventImmediate(hitEvent);
}
