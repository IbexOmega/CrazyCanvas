#include "EventHandlers/PingHandler.h"

#include "Application/API/Events/EventQueue.h"
#include "Application/API/Events/KeyEvents.h"
#include "ECS/Components/GUI/ProjectedGUIComponent.h"
#include "ECS/Components/Misc/DestructionComponent.h"
#include "ECS/ECSCore.h"
#include "Game/ECS/Components/Misc/InheritanceComponent.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Team/TeamComponent.h"
#include "Game/ECS/Systems/Physics/PhysicsSystem.h"
#include "Input/API/InputActionSystem.h"
#include "Lobby/PlayerManagerClient.h"
#include "Resources/ResourceCatalog.h"
#include "Resources/ResourceManager.h"

PingHandler::~PingHandler()
{
	using namespace LambdaEngine;
	EventQueue::UnregisterEventHandler<KeyPressedEvent>(this, &PingHandler::OnKeyPress);
}

void PingHandler::Init()
{
	using namespace LambdaEngine;

	m_pPingSound = ResourceManager::GetSoundEffect3D(ResourceCatalog::SOUND_EFFECT_PING);

	EntitySubscriberRegistration subscriberReg;
	subscriberReg.EntitySubscriptionRegistrations =
	{
		{
			.pSubscriber = &m_LocalPlayerCamera,
			.ComponentAccesses =
			{
				{ NDA, CameraComponent::Type() },
				{ NDA, PositionComponent::Type() },
				{ NDA, RotationComponent::Type() }
			}
		}
	};

	SubscribeToEntities(subscriberReg);

	EventQueue::RegisterEventHandler<KeyPressedEvent>(this, &PingHandler::OnKeyPress);
}

bool PingHandler::OnKeyPress(const LambdaEngine::KeyPressedEvent& keyPressEvent)
{
	using namespace LambdaEngine;

	if (InputActionSystem::IsActionBoundToKey(EAction::ACTION_GENERAL_PING, keyPressEvent.Key))
	{
		const Job pingJob =
		{
			.Components =
			{
				/*	Component accesses are for:
					1. Transform of the camera entity is read when raycasting.
					2. Creating a ping entity. */
				{ RW, PositionComponent::Type() },
				{ R, RotationComponent::Type() },
				{ RW, TeamComponent::Type() },
				{ RW, ProjectedGUIComponent::Type() },
				{ RW, DestructionComponent::Type() },
				{ RW, ParentComponent::Type() }
			},
			.Function = [this]()
			{
				ECSCore* pECS = ECSCore::GetInstance();

				const Entity camEntity = m_LocalPlayerCamera.Front();
				const glm::vec3& camPos = pECS->GetConstComponent<PositionComponent>(camEntity).Position;
				const glm::vec3 camDir = GetForward(pECS->GetConstComponent<RotationComponent>(camEntity).Quaternion);

				PhysicsSystem* pPhysicsSystem = PhysicsSystem::GetInstance();

				const RaycastFilterData rayFilterData =
				{
					.IncludedGroup = FCollisionGroup::COLLISION_GROUP_STATIC
				};

				PxRaycastHit raycastHit;
				if (pPhysicsSystem->Raycast(camPos, camDir, MAX_PING_DISTANCE, raycastHit, &rayFilterData))
				{
					/*	The player was looking at static geometry whilst pressing the ping button. Either create a ping
						marker, or if the player currently has another ping, reposition that one.  */
					bool createNewPing = true;

					const Player* pLocalPlayer = PlayerManagerClient::GetPlayerLocal();
					const Entity playerEntity = pLocalPlayer->GetEntity();
					const glm::vec3 pingPos = { raycastHit.position.x, raycastHit.position.y, raycastHit.position.z };

					if (m_PlayerPings.HasElement(playerEntity))
					{
						/*	The player has already placed a ping, see if the old one still exists by inspecting the old
							ping entity's components. */
						const Entity previousPingEntity = m_PlayerPings.IndexID(playerEntity);
						ComponentArray<ProjectedGUIComponent>* pProjectedGUIComponents = pECS->GetComponentArray<ProjectedGUIComponent>();
						ComponentArray<ParentComponent>* pParentComponents = pECS->GetComponentArray<ParentComponent>();

						ProjectedGUIComponent projectedGUIComponent;
						ParentComponent parentComponent;
						if (pProjectedGUIComponents->GetConstIf(previousPingEntity, projectedGUIComponent) &&
							projectedGUIComponent.GUIType == IndicatorTypeGUI::PING_INDICATOR &&
							pParentComponents->GetConstIf(previousPingEntity, parentComponent) &&
							parentComponent.Parent == playerEntity)
						{
							createNewPing = false;

							// The old ping still exists, edit it as opposed to creating a new one
							pECS->GetComponent<PositionComponent>(previousPingEntity).Position = pingPos;
							pECS->GetComponent<DestructionComponent>(previousPingEntity).TimeLeft = PING_DURATION;
						}
					}

					if (createNewPing)
					{
						const Entity pingEntity = pECS->CreateEntity();
						pECS->AddComponent(pingEntity, PositionComponent({ .Position = pingPos }));
						pECS->AddComponent(pingEntity, TeamComponent({ .TeamIndex = pLocalPlayer->GetTeam() }));
						pECS->AddComponent(pingEntity, ProjectedGUIComponent({ .GUIType = IndicatorTypeGUI::PING_INDICATOR }));
						pECS->AddComponent(pingEntity, DestructionComponent({ .TimeLeft = PING_DURATION }));
						pECS->AddComponent(pingEntity, ParentComponent({ .Parent = playerEntity }));

						m_PlayerPings.PushBack(pingEntity, playerEntity);
					}

					// Play the ping sound 1 meter away from the camera so that there is directional sound, but minimal attenuation
					m_pPingSound->PlayOnceAt(camPos + glm::normalize(pingPos - camPos));
				}
			}
		};

		ECSCore::GetInstance()->ScheduleJobASAP(pingJob);
	}

	return false;
}
