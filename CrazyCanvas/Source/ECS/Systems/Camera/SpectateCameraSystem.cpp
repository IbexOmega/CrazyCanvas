#include "ECS/Systems/Camera/SpectateCameraSystem.h"

#include "Application/API/Events/EventQueue.h"

#include "ECS/ECSCore.h"
#include "ECS/Components/Player/Player.h"
#include "ECS/Components/Match/FlagComponent.h"

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
	EntitySubscriberRegistration entityReg = {};
	entityReg.EntitySubscriptionRegistrations =
	{
		{
			.pSubscriber = &m_CameraEntities,
			.ComponentAccesses =
			{
				{ RW, CameraComponent::Type() }, 
				{ RW, OffsetComponent::Type() },
				{ RW, ParentComponent::Type() }
			}
		},
		{
			.pSubscriber = &m_FlagSpawnEntities,
			.ComponentAccesses =
			{
				{ R, TeamComponent::Type() },
				{ NDA, FlagSpawnComponent::Type() },
			}
		}
	};

	SubscribeToEntities(entityReg);

	EventQueue::RegisterEventHandler<MouseButtonClickedEvent>(this, &SpectateCameraSystem::OnMouseButtonClicked);
	EventQueue::RegisterEventHandler<PlayerAliveUpdatedEvent>(this, &SpectateCameraSystem::OnPlayerAliveUpdated);

	m_LocalTeamIndex = PlayerManagerClient::GetPlayerLocal()->GetTeam();
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
	using namespace LambdaEngine;

	ECSCore* pECS = ECSCore::GetInstance();

	Job job;
	job.Components =
	{
		{ RW, CameraComponent::Type() },
		{ RW, OffsetComponent::Type() },
		{ RW, ParentComponent::Type() }
	};

	job.Function = [this, event]()
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
					RenderSystem::GetInstance().SetRenderStageSleeping("RENDER_STAGE_FIRST_PERSON_WEAPON", false);
					ParentComponent& parentComponent = pParentComponents->GetData(cameraEntity);
					OffsetComponent& cameraOffsetComponent = pOffsetComponents->GetData(cameraEntity);

					cameraOffsetComponent.Offset *= 0.5f; // reset camera offset
					parentComponent.Parent = PlayerManagerClient::GetPlayerLocal()->GetEntity(); // reset camera parent
				}

				m_SpectatorIndex = 0;
				m_InSpectateView = false;
				m_pSpectatedPlayer = nullptr;
			}
			else
			{
				for (Entity cameraEntity : m_CameraEntities)
				{
					RenderSystem::GetInstance().SetRenderStageSleeping("RENDER_STAGE_FIRST_PERSON_WEAPON", true);
					OffsetComponent& cameraOffsetComponent = pOffsetComponents->GetData(cameraEntity);

					cameraOffsetComponent.Offset *= 2;
					SpectatePlayer();
				}
				m_InSpectateView = true;
			}
		}
		else if (m_pSpectatedPlayer == event.pPlayer)
		{
			SpectatePlayer();
		}
	};

	pECS->ScheduleJobASAP(job);

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

		for (int i = teamPlayers.GetSize() - 1; i >= 0; i--)
		{
			if (teamPlayers[i]->GetEntity() == localPlayer) //remove local player from list
			{
				teamPlayers.Erase(teamPlayers.begin() + i);
			}
			else if (teamPlayers[i]->IsDead()) //remove dead players from list
			{
				teamPlayers.Erase(teamPlayers.begin() + i);
			}
		}

		if (teamPlayers.GetSize() > 0) //Spectate team-member
		{
			for (Entity cameraEntity : m_CameraEntities)
			{
				ParentComponent& parentComponent = pParentComponents->GetData(cameraEntity);

				m_SpectatorIndex = m_SpectatorIndex % teamPlayers.GetSize();

				m_pSpectatedPlayer = teamPlayers[m_SpectatorIndex];

				Entity nextPlayer = m_pSpectatedPlayer->GetEntity();
				parentComponent.Parent = nextPlayer;
			}
		}
		else // spectate Flag
		{
			const ComponentArray<TeamComponent>* pTeamComponents = pECS->GetComponentArray<TeamComponent>();

			for (Entity cameraEntity : m_CameraEntities)
			{
				ParentComponent& parentComponent = pParentComponents->GetData(cameraEntity);

				for (Entity flagSpawnEntity : m_FlagSpawnEntities)
				{
					const TeamComponent& teamComponent = pTeamComponents->GetConstData(flagSpawnEntity);
					if (teamComponent.TeamIndex == m_LocalTeamIndex)
					{
						parentComponent.Parent = flagSpawnEntity;
					}
				}
			}
		}
	}
}
