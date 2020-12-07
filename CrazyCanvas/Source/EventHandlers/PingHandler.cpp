#include "EventHandlers/PingHandler.h"

#include "Application/API/Events/EventQueue.h"
#include "Application/API/Events/KeyEvents.h"
#include "ECS/Components/GUI/ProjectedGUIComponent.h"
#include "ECS/Components/Misc/DestructionComponent.h"
#include "ECS/ECSCore.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Team/TeamComponent.h"
#include "Game/ECS/Systems/Physics/PhysicsSystem.h"
#include "Input/API/InputActionSystem.h"
#include "Lobby/PlayerManagerClient.h"

PingHandler::~PingHandler()
{
	using namespace LambdaEngine;
	EventQueue::UnregisterEventHandler<KeyPressedEvent>(this, &PingHandler::OnKeyPress);
}

void PingHandler::Init()
{
	using namespace LambdaEngine;

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
				{ RW, DestructionComponent::Type() }
			},
			.Function = [this]()
			{
				ECSCore* pECS = ECSCore::GetInstance();

				const Player* pLocalPlayer = PlayerManagerClient::GetPlayerLocal();
				const Entity camEntity = pLocalPlayer->GetEntity();
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
					// The player was looking at static geometry whilst pressing the ping button. Create the ping marker.
					const Entity pingEntity = pECS->CreateEntity();
					pECS->AddComponent(pingEntity, PositionComponent({ .Position = camPos }));
					pECS->AddComponent(pingEntity, TeamComponent({ .TeamIndex = pLocalPlayer->GetTeam() }));
					pECS->AddComponent(pingEntity, ProjectedGUIComponent({ .GUIType = IndicatorTypeGUI::PING_INDICATOR }));
					pECS->AddComponent(pingEntity, DestructionComponent({ .TimeLeft = PING_DURATION }));
				}
			}
		};

		ECSCore::GetInstance()->ScheduleJobASAP(pingJob);
	}

	return false;
}
