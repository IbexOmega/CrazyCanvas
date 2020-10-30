#include "ECS/Systems/Player/BenchmarkSystem.h"

#include "ECS/Components/Player/Player.h"
#include "ECS/Components/Player/WeaponComponent.h"
#include "ECS/Components/Team/TeamComponent.h"
#include "ECS/ECSCore.h"
#include "ECS/Systems/Player/WeaponSystem.h"

#include "Game/ECS/Systems/Physics/PhysicsSystem.h"
#include "World/Player/CharacterControllerHelper.h"

#include "Input/API/Input.h"

#include "Physics/PhysicsEvents.h"

#include "Resources/Material.h"
#include "Resources/ResourceManager.h"

void BenchmarkSystem::Init()
{
	using namespace LambdaEngine;

	// Register system
	{
		// The write permissions are used when creating projectile entities
		PlayerGroup playerGroup;
		playerGroup.Rotation.Permissions = RW;

		SystemRegistration systemReg = {};
		systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
		{
			{
				.pSubscriber = &m_WeaponEntities,
				.ComponentAccesses =
				{
					{ RW, WeaponComponent::Type() },
					{ R, PositionComponent::Type() },
					{ R, RotationComponent::Type() },
					{ RW, PacketComponent<WeaponFiredPacket>::Type() }
				}
			},
			{
				.pSubscriber = &m_LocalPlayerEntities,
				.ComponentAccesses =
				{
					{ RW, CharacterColliderComponent::Type() },
					{ R, NetworkPositionComponent::Type() },
					{ RW, VelocityComponent::Type() }
				},
				.ComponentGroups = { &playerGroup }
			}
		};
		systemReg.Phase = 0u;

		RegisterSystem(TYPE_NAME(BenchmarkSystem), systemReg);
	}
}

void BenchmarkSystem::Tick(LambdaEngine::Timestamp deltaTime)
{
	using namespace LambdaEngine;
	const float32 dt = (float32)deltaTime.AsSeconds();

	ECSCore* pECS = ECSCore::GetInstance();

	ComponentArray<WeaponComponent>* pWeaponComponents				= pECS->GetComponentArray<WeaponComponent>();
	const ComponentArray<PositionComponent>* pPositionComponents	= pECS->GetComponentArray<PositionComponent>();
	ComponentArray<RotationComponent>* pRotationComponents			= pECS->GetComponentArray<RotationComponent>();

	/* Component arrays for moving character colliders */
	ComponentArray<CharacterColliderComponent>* pCharacterColliderComponents	= pECS->GetComponentArray<CharacterColliderComponent>();
	const ComponentArray<NetworkPositionComponent>* pNetworkPositionComponents	= pECS->GetComponentArray<NetworkPositionComponent>();
	ComponentArray<VelocityComponent>* pVelocityComponents						= pECS->GetComponentArray<VelocityComponent>();

	WeaponSystem& weaponSystem = WeaponSystem::GetInstance();
	for (Entity weaponEntity : m_WeaponEntities)
	{
		WeaponComponent& weaponComponent = pWeaponComponents->GetData(weaponEntity);
		const Entity playerEntity = weaponComponent.WeaponOwner;
		if (!m_LocalPlayerEntities.HasElement(playerEntity))
		{
			continue;
		}

		// Rotate player
		constexpr const float32 rotationSpeed = 3.14f / 4.0f;
		RotationComponent& rotationComp = pRotationComponents->GetData(playerEntity);
		rotationComp.Quaternion = glm::rotate(rotationComp.Quaternion, rotationSpeed * dt, g_DefaultUp);

		// Move character controller to apply gravity and to stress the physics system
		CharacterControllerHelper::TickCharacterController(dt, playerEntity, pCharacterColliderComponents, pNetworkPositionComponents, pVelocityComponents);

		/* Fire weapon if appropriate */
		if (weaponComponent.CurrentCooldown > 0.0f)
		{
			weaponComponent.CurrentCooldown -= dt;
			continue;
		}

		weaponSystem.Fire(
			EAmmoType::AMMO_TYPE_PAINT,
			playerEntity,
			pPositionComponents->GetConstData(weaponEntity).Position,
			pRotationComponents->GetConstData(playerEntity).Quaternion,
			pVelocityComponents->GetConstData(playerEntity).Velocity
		);

		weaponComponent.CurrentCooldown = 1.0f / weaponComponent.FireRate;
	}
}
