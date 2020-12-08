#include "ECS/Systems/GUI/NameplateSystem.h"

#include "ECS/ECSCore.h"
#include "ECS/Systems/GUI/HUDSystem.h"
#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Player/PlayerComponent.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Team/TeamComponent.h"
#include "Game/ECS/Systems/Physics/PhysicsSystem.h"
#include "Lobby/PlayerManagerClient.h"
#include "Physics/CollisionGroups.h"

NameplateSystem::NameplateSystem(HUDSystem* pHUDSystem)
	: m_pHUDSystem(pHUDSystem)
{}

void NameplateSystem::Init()
{
	using namespace LambdaEngine;

	// Register system
	{
		EntitySubscriberRegistration subscriberReg;
		subscriberReg.EntitySubscriptionRegistrations =
		{
			{
				.pSubscriber = &m_ForeignPlayers,
				.ComponentAccesses =
				{
					{ NDA, PlayerForeignComponent::Type() },
					{ R, TeamComponent::Type() }
				}
			},
			{
				.pSubscriber = &m_LocalPlayerCamera,
				.ComponentAccesses =
				{
					{ NDA, CameraComponent::Type() },
					{ R, PositionComponent::Type() },
					{ R, RotationComponent::Type() }
				}
			}
		};

		SubscribeToEntities(subscriberReg);
	}

}

void NameplateSystem::FixedTick(LambdaEngine::Timestamp deltaTime)
{
	using namespace LambdaEngine;

	if (m_LocalPlayerCamera.Empty())
	{
		return;
	}

	const Player* pLocalPlayer = PlayerManagerClient::GetPlayerLocal();

	// Fetch the local player camera's transform and perform a raycast in its looking direction
	const Entity localPlayerCamera = m_LocalPlayerCamera.Front();

	ECSCore* pECS = ECSCore::GetInstance();
	const PositionComponent& positionComp = pECS->GetConstComponent<PositionComponent>(localPlayerCamera);
	const RotationComponent& rotationComp = pECS->GetConstComponent<RotationComponent>(localPlayerCamera);

	const glm::vec3 rayDir = GetForward(rotationComp.Quaternion);
	physx::PxRaycastHit raycastHit;

	const RaycastFilterData raycastFilterData =
	{
		.IncludedGroup = (uint32)FCrazyCanvasCollisionGroup::COLLISION_GROUP_PLAYER | (uint32)FCollisionGroup::COLLISION_GROUP_STATIC,
		.ExcludedGroup = FCrazyCanvasCollisionGroup::COLLISION_GROUP_PROJECTILE,
		.ExcludedEntity = pLocalPlayer->GetEntity()
	};

	if (PhysicsSystem::GetInstance()->Raycast(positionComp.Position, rayDir, MAX_NAMEPLATE_DISTANCE, raycastHit, &raycastFilterData))
	{
		// The ray hit something, find out if the hit actor belongs to a teammate
		const ActorUserData* pHitActorUserData = reinterpret_cast<const ActorUserData*>(raycastHit.actor->userData);
		const Entity hitEntity = pHitActorUserData->Entity;

		ComponentArray<TeamComponent>* pTeamComponents = pECS->GetComponentArray<TeamComponent>();
		const uint8 localPlayerTeam = pLocalPlayer->GetTeam();

		for (Entity foreignPlayer : m_ForeignPlayers)
		{
			if (hitEntity == foreignPlayer)
			{
				const TeamComponent& teamComponent = pTeamComponents->GetConstData(foreignPlayer);
				if (teamComponent.TeamIndex == localPlayerTeam)
				{
					// The local player is looking at a teammate
					const Player* pPlayer = PlayerManagerClient::GetPlayer(foreignPlayer);
					if (pPlayer)
					{
						// No need to tell the HUD to display the nameplate if it's already visible
						if (hitEntity != m_PreviouslyViewedTeammate)
						{
							m_pHUDSystem->DisplayNamePlate(pPlayer->GetName(), true);
							m_PreviouslyViewedTeammate = hitEntity;
						}

						return;
					}
				}
			}
		}
	}

	// The local player is not looking at a teammate, hide the nameplate
	if (m_PreviouslyViewedTeammate != UINT32_MAX)
	{
		m_PreviouslyViewedTeammate = UINT32_MAX;
		m_pHUDSystem->DisplayNamePlate("", false);
	}
}
