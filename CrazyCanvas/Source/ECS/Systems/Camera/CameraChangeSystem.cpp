#include "ECS/Systems/Camera/CameraChangeSystem.h"

#include "ECS/ECSCore.h"

#include "Application/API/Events/EventQueue.h"

#include "ECS/Components/Player/Player.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Misc/InheritanceComponent.h"

#include "Lobby/Player.h"

#include "Lobby/PlayerManagerClient.h"


using namespace LambdaEngine;

CameraChangeSystem::~CameraChangeSystem()
{
	EventQueue::UnregisterEventHandler<MouseButtonClickedEvent>(this, &CameraChangeSystem::OnMouseButtonClicked);
	EventQueue::UnregisterEventHandler<PlayerAliveUpdatedEvent>(this, &CameraChangeSystem::OnPlayerAliveUpdated);
}

void CameraChangeSystem::Init()
{
	SystemRegistration systemReg = {};
	systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
	{
		{
			.pSubscriber = &m_PlayerEntities,
			.ComponentAccesses =
			{
				{ NDA, PlayerLocalComponent::Type() }, { RW, CameraComponent::Type() }
			}
		},
		{
			.pSubscriber = &m_CameraEntities,
			.ComponentAccesses =
			{
				{ RW, CameraComponent::Type() }
			}
		}
	};

	systemReg.Phase = 1;

	RegisterSystem(TYPE_NAME(HUDSystem), systemReg);
	EventQueue::RegisterEventHandler<MouseButtonClickedEvent>(this, &CameraChangeSystem::OnMouseButtonClicked);
	EventQueue::RegisterEventHandler<PlayerAliveUpdatedEvent>(this, &CameraChangeSystem::OnPlayerAliveUpdated);

	m_LocalTeamIndex = PlayerManagerClient::GetPlayerLocal()->GetTeam();

}

void CameraChangeSystem::Tick(LambdaEngine::Timestamp deltaTime)
{
	UNREFERENCED_VARIABLE(deltaTime);
}

void CameraChangeSystem::FixedTick(LambdaEngine::Timestamp deltaTime)
{

	UNREFERENCED_VARIABLE(deltaTime);
}


bool CameraChangeSystem::OnMouseButtonClicked(const MouseButtonClickedEvent& event)
{
	if (PlayerManagerClient::GetPlayerLocal()->IsDead())
	{
		PlayerManagerClient::GetPlayersOfTeam(m_TeamPlayers, m_LocalTeamIndex);

		if (!m_TeamPlayers.IsEmpty())
		{
			ECSCore* pECS = ECSCore::GetInstance();
			ComponentArray<ParentComponent>* pParentComponents = pECS->GetComponentArray<ParentComponent>();

			for (Entity cameraEntity : m_CameraEntities)
			{
				ParentComponent& parentComponent = pParentComponents->GetData(cameraEntity);
				
				Entity nextPlayer = m_TeamPlayers[m_SpectatorIndex]->GetEntity();

				if (event.Button == EMouseButton::MOUSE_BUTTON_RIGHT)
				{
					m_SpectatorIndex++;
					nextPlayer = nextPlayer != PlayerManagerClient::GetPlayerLocal()->GetEntity() ? nextPlayer : m_TeamPlayers[m_SpectatorIndex + 1]->GetEntity();
					parentComponent.Parent = nextPlayer;
					m_SpectatorIndex %= m_TeamPlayers.GetSize();

				}
				else if (event.Button == EMouseButton::MOUSE_BUTTON_LEFT)
				{
					m_SpectatorIndex--;
					nextPlayer = nextPlayer != PlayerManagerClient::GetPlayerLocal()->GetEntity() ? nextPlayer : m_TeamPlayers[m_SpectatorIndex + 1]->GetEntity();
					parentComponent.Parent = nextPlayer;
					m_SpectatorIndex %= m_TeamPlayers.GetSize();
				}
			}
		}
	}

	return false;
}

bool CameraChangeSystem::OnPlayerAliveUpdated(const PlayerAliveUpdatedEvent& event)
{
	if (!PlayerManagerClient::GetPlayerLocal()->IsDead())
	{
		ECSCore* pECS = ECSCore::GetInstance();
		ComponentArray<ParentComponent>* pParentComponents = pECS->GetComponentArray<ParentComponent>();

		for (Entity cameraEntity : m_CameraEntities)
		{
			ParentComponent& parentComponent = pParentComponents->GetData(cameraEntity);
			parentComponent.Parent = PlayerManagerClient::GetPlayerLocal()->GetEntity();
		}
		m_SpectatorIndex = 0;
	}
	return false;
}
