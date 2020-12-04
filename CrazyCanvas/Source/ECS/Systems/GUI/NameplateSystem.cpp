#include "ECS/Systems/GUI/NameplateSystem.h"

#include "ECS/ECSCore.h"
#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Player/PlayerComponent.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Team/TeamComponent.h"
#include "Game/ECS/Systems/Physics/PhysicsSystem.h"
#include "Lobby/PlayerManagerClient.h"

NameplateSystem::NameplateSystem(HUDSystem* pHUDSystem)
	: m_pHUDSystem(pHUDSystem)
{}

void NameplateSystem::Init()
{
	using namespace LambdaEngine;

	// Register system
	{
		SystemRegistration systemReg = {};
		systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
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

		// Should not run concurrently with PhysicsSystem as ray queries result in reads from the physx scene
		systemReg.Phase = 2;
		RegisterSystem(TYPE_NAME(NameplateSystem), systemReg);
	}

	m_LocalPlayerTeam = PlayerManagerClient::GetPlayerLocal()->GetTeam();
}

void NameplateSystem::Tick(LambdaEngine::Timestamp deltaTime)
{
	using namespace LambdaEngine;

	if (m_LocalPlayerCamera.Empty())
	{
		return;
	}

	// Fetch the local player camera's transform and perform a raycast in its looking direction
	const Entity localPlayer = m_LocalPlayerCamera.Front();

	ECSCore* pECS = ECSCore::GetInstance();
	const PositionComponent& positionComp = pECS->GetConstComponent<PositionComponent>(localPlayer);
	const RotationComponent& rotationComp = pECS->GetConstComponent<RotationComponent>(localPlayer);

	const glm::vec3 rayDir = GetForward(rotationComp.Quaternion);
	physx::PxRaycastHit raycastHit;

	if (PhysicsSystem::GetInstance()->Raycast(positionComp.Position, rayDir, MAX_NAMEPLATE_DISTANCE, raycastHit))
	{
		// The ray hit something, find out if the hit actor belongs to a teammate
		const ActorUserData* pHitActorUserData = reinterpret_cast<const ActorUserData*>(raycastHit.actor->userData);
		const Entity hitEntity = pHitActorUserData->Entity;

		ComponentArray<TeamComponent>* pTeamComponents = pECS->GetComponentArray<TeamComponent>();

		for (Entity foreignPlayer : m_ForeignPlayers)
		{
			if (hitEntity == foreignPlayer)
			{
				const TeamComponent& teamComponent = pTeamComponents->GetConstData(foreignPlayer);
				if (teamComponent.TeamIndex == m_LocalPlayerTeam)
				{
					LOG_INFO("Looking at teammate");
					// m_pHUDSystem->ActivateNameplate(...);
				}
			}
		}
	}
}
