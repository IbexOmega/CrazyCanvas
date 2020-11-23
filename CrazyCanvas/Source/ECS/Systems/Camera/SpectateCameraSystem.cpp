#include "ECS/Systems/Camera/SpectateCameraSystem.h"

#include "ECS/ECSCore.h"

#include "Application/API/Events/EventQueue.h"

#include "ECS/Components/Player/Player.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Misc/InheritanceComponent.h"
#include "Game/ECS/Components/Physics/Transform.h"

#include "Lobby/Player.h"

#include "Lobby/PlayerManagerClient.h"


using namespace LambdaEngine;

SpectateCameraSystem::~SpectateCameraSystem()
{
	EventQueue::UnregisterEventHandler<MouseButtonClickedEvent>(this, &SpectateCameraSystem::OnMouseButtonClicked);
	EventQueue::UnregisterEventHandler<PlayerAliveUpdatedEvent>(this, &SpectateCameraSystem::OnPlayerAliveUpdated);
}

void SpectateCameraSystem::Init()
{
	SystemRegistration systemReg = {};
	systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
	{
		{
			.pSubscriber = &m_PlayerEntities,
			.ComponentAccesses =
			{
				{ NDA, PlayerLocalComponent::Type() }
			}
		},
		{
			.pSubscriber = &m_CameraEntities,
			.ComponentAccesses =
			{
				{ RW, CameraComponent::Type() }, { RW, OffsetComponent::Type() }
			}
		}
	};

	systemReg.SubscriberRegistration.AdditionalAccesses =
	{
		{ R, ScaleComponent::Type() }
	};

	systemReg.Phase = 1;

	RegisterSystem(TYPE_NAME(HUDSystem), systemReg);
	EventQueue::RegisterEventHandler<MouseButtonClickedEvent>(this, &SpectateCameraSystem::OnMouseButtonClicked);
	EventQueue::RegisterEventHandler<PlayerAliveUpdatedEvent>(this, &SpectateCameraSystem::OnPlayerAliveUpdated);

	m_LocalTeamIndex = PlayerManagerClient::GetPlayerLocal()->GetTeam();

}

void SpectateCameraSystem::Tick(LambdaEngine::Timestamp deltaTime)
{
	UNREFERENCED_VARIABLE(deltaTime);
}

void SpectateCameraSystem::FixedTick(LambdaEngine::Timestamp deltaTime)
{

	UNREFERENCED_VARIABLE(deltaTime);
}


bool SpectateCameraSystem::OnMouseButtonClicked(const MouseButtonClickedEvent& event)
{
	if (m_InSpectateView)
	{
		if (event.Button == EMouseButton::MOUSE_BUTTON_RIGHT)
		{
			m_SpectatorIndex++;
			SpectatePlayer();
		}
		else if (event.Button == EMouseButton::MOUSE_BUTTON_LEFT)
		{
			m_SpectatorIndex--;
			SpectatePlayer();
		}
	}

	return false;
}

bool SpectateCameraSystem::OnPlayerAliveUpdated(const PlayerAliveUpdatedEvent& event)
{
	const Player* pLocalPlayer = PlayerManagerClient::GetPlayerLocal();

	if (pLocalPlayer == event.pPlayer)
	{
		ECSCore* pECS = ECSCore::GetInstance();
		ComponentArray<ParentComponent>* pParentComponents = pECS->GetComponentArray<ParentComponent>();
		ComponentArray<OffsetComponent>* pOffsetComponents = pECS->GetComponentArray<OffsetComponent>();

		if (!pLocalPlayer->IsDead())
		{
			for (Entity cameraEntity : m_CameraEntities)
			{
				ParentComponent& parentComponent = pParentComponents->GetData(cameraEntity);
				OffsetComponent& cameraOffsetComponent = pOffsetComponents->GetData(cameraEntity);

				cameraOffsetComponent.Offset *= 0.5; // reset camera offset
				parentComponent.Parent = PlayerManagerClient::GetPlayerLocal()->GetEntity(); // reset camera parent
			}

			m_SpectatorIndex = 0;
			m_InSpectateView = false;
		}
		else
		{
			for (Entity cameraEntity : m_CameraEntities)
			{
				ParentComponent& parentComponent = pParentComponents->GetData(cameraEntity);
				OffsetComponent& cameraOffsetComponent = pOffsetComponents->GetData(cameraEntity);

				cameraOffsetComponent.Offset *= 2;
				parentComponent.Parent = PlayerManagerClient::GetPlayerLocal()->GetEntity();

			}

			SpectatePlayer();
			m_InSpectateView = true;
		}
	}
	return false;
}

void SpectateCameraSystem::SpectatePlayer()
{
	LambdaEngine::TArray<const Player*> teamPlayers;
	PlayerManagerClient::GetPlayersOfTeam(teamPlayers, m_LocalTeamIndex);

	if (!teamPlayers.IsEmpty())
	{
		ECSCore* pECS = ECSCore::GetInstance();
		ComponentArray<ParentComponent>* pParentComponents = pECS->GetComponentArray<ParentComponent>();

		Entity localPlayer = PlayerManagerClient::GetPlayerLocal()->GetEntity();

		for (uint32 i = 0; i < teamPlayers.GetSize(); i++)
		{
			if (teamPlayers[i]->GetEntity() == localPlayer)
			{
				teamPlayers.Erase(teamPlayers.begin() + i);
				break;
			}
		}

		if (teamPlayers.GetSize() > 0)
		{
			for (Entity cameraEntity : m_CameraEntities)
			{
				ParentComponent& parentComponent = pParentComponents->GetData(cameraEntity);

				m_SpectatorIndex = m_SpectatorIndex % teamPlayers.GetSize();

				Entity nextPlayer = teamPlayers[m_SpectatorIndex]->GetEntity();
				parentComponent.Parent = nextPlayer;

				/*LOG_INFO("Jumping to player entity %d", nextPlayer);
				LOG_INFO("Current m_SpectatorIndex %d", m_SpectatorIndex);*/
			}
		}
	}
}
