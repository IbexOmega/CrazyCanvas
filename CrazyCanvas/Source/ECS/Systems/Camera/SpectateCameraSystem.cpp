#include "ECS/Systems/Camera/SpectateCameraSystem.h"

#include "Application/API/Events/EventQueue.h"

#include "ECS/ECSCore.h"
#include "ECS/Components/Player/Player.h"
#include "ECS/Components/Match/FlagComponent.h"
#include "ECS/Components/World/SpectateComponent.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Misc/InheritanceComponent.h"
#include "Game/ECS/Components/Physics/Transform.h"

#include "Events/GameplayEvents.h"

#include "Lobby/Player.h"

#include "Lobby/PlayerManagerClient.h"

using namespace LambdaEngine;

SpectateCameraSystem::~SpectateCameraSystem()
{
	EventQueue::UnregisterEventHandler<MouseButtonClickedEvent>(this, &SpectateCameraSystem::OnMouseButtonClicked);
	EventQueue::UnregisterEventHandler<PlayerAliveUpdatedEvent>(this, &SpectateCameraSystem::OnPlayerAliveUpdated);
	EventQueue::UnregisterEventHandler<GameOverEvent>(this, &SpectateCameraSystem::OnGameOver);

}

void SpectateCameraSystem::Init()
{
	SystemRegistration systemReg = {};
	systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
	{
		{
			.pSubscriber = & m_SpectatableEntities,
			.ComponentAccesses = 
			{
				{ NDA, SpectateComponent::Type() }
			}
		}
	};

	systemReg.SubscriberRegistration.AdditionalAccesses =
	{
		{ RW, CameraComponent::Type() },
		{ RW, OffsetComponent::Type() },
		{ RW, ParentComponent::Type() },
		{ RW, RotationComponent::Type() },
		{ R, TeamComponent::Type() },
		{ NDA, FlagSpawnComponent::Type() },
	};

	RegisterSystem(TYPE_NAME(SpectateCameraSystem), systemReg);

	EventQueue::RegisterEventHandler<MouseButtonClickedEvent>(this, &SpectateCameraSystem::OnMouseButtonClicked);
	EventQueue::RegisterEventHandler<PlayerAliveUpdatedEvent>(this, &SpectateCameraSystem::OnPlayerAliveUpdated);
	EventQueue::RegisterEventHandler<GameOverEvent>(this, &SpectateCameraSystem::OnGameOver);

	m_LocalTeamIndex = PlayerManagerClient::GetPlayerLocal()->GetTeam();
}

void SpectateCameraSystem::Tick(LambdaEngine::Timestamp deltaTime)
{

	UNREFERENCED_VARIABLE(deltaTime);
	//After tomorrow I will add a neat rotation to the camera upon death. dont have time to fix it now
	/*if (m_IsGameOver)
	{

	}*/
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
			LOG_ERROR("index before: %u", m_SpectatorIndex);
			m_SpectatorIndex--;
			SpectatePlayer();
			LOG_ERROR("index before: %u", m_SpectatorIndex);
		}
	}

	return false;
}

bool SpectateCameraSystem::OnPlayerAliveUpdated(const PlayerAliveUpdatedEvent& event)
{
	using namespace LambdaEngine;

	const Player* pLocalPlayer = PlayerManagerClient::GetPlayerLocal();

	if (pLocalPlayer == event.pPlayer)
	{
		ECSCore* pECS = ECSCore::GetInstance();
		ComponentArray<ParentComponent>* pParentComponents = pECS->GetComponentArray<ParentComponent>();
		ComponentArray<OffsetComponent>* pOffsetComponents = pECS->GetComponentArray<OffsetComponent>();
		ComponentArray<CameraComponent>* pCameraComponents = pECS->GetComponentArray<CameraComponent>();

		Entity cameraEntity = UINT32_MAX;

		for (Entity entity : m_SpectatableEntities)
		{
			if (pCameraComponents->HasComponent(entity))
				cameraEntity = entity;		
		}

		if (cameraEntity != UINT32_MAX)
		{
			if (!pLocalPlayer->IsDead())
			{
				RenderSystem::GetInstance().SetRenderStageSleeping("RENDER_STAGE_FIRST_PERSON_WEAPON", false);
				ParentComponent& parentComponent = pParentComponents->GetData(cameraEntity);
				OffsetComponent& cameraOffsetComponent = pOffsetComponents->GetData(cameraEntity);
				cameraOffsetComponent.Offset *= 0.5f; // reset camera offset
				parentComponent.Parent = PlayerManagerClient::GetPlayerLocal()->GetEntity(); // reset camera parent

				m_SpectatorIndex = 0;
				m_InSpectateView = false;
				m_SpectatedPlayer = UINT32_MAX;

				SpectatePlayerEvent spectatePlayerEvent("", false);
				EventQueue::SendEventImmediate(spectatePlayerEvent);
			}
			else
			{
				RenderSystem::GetInstance().SetRenderStageSleeping("RENDER_STAGE_FIRST_PERSON_WEAPON", true);
				OffsetComponent& cameraOffsetComponent = pOffsetComponents->GetData(cameraEntity);

				cameraOffsetComponent.Offset *= 2;
				SpectatePlayer();
				m_InSpectateView = true;
			}
		}
	}
	else if (m_SpectatedPlayer == event.pPlayer->GetEntity())
	{
		SpectatePlayer();
	}

	return false;
}

bool SpectateCameraSystem::OnGameOver(const GameOverEvent& event)
{

	UNREFERENCED_VARIABLE(event);

	m_IsGameOver = true;

	SpectatePlayer();

	return false;
}

void SpectateCameraSystem::SpectatePlayer()
{
	LambdaEngine::TArray<Entity> teamPlayers;

	ECSCore* pECS = ECSCore::GetInstance();

	ComponentArray<ParentComponent>* pParentComponents = pECS->GetComponentArray<ParentComponent>();
	const ComponentArray<CameraComponent>* pCameraComponents = pECS->GetComponentArray<CameraComponent>();
	const ComponentArray<FlagSpawnComponent>* pFlagSpawnComponents = pECS->GetComponentArray<FlagSpawnComponent>();
	const ComponentArray<TeamComponent>* pTeamComponents = pECS->GetComponentArray<TeamComponent>();

	Entity localPlayer = PlayerManagerClient::GetPlayerLocal()->GetEntity();
	Entity cameraEntity = UINT32_MAX;
	Entity flagSpawnEntity = UINT32_MAX;
	Entity mapSpectatePointEntity = UINT32_MAX;

	for (Entity entity : m_SpectatableEntities)
	{
		const Player* pPlayer = PlayerManagerClient::GetPlayer(entity);

		if (pCameraComponents->HasComponent(entity))
		{
			cameraEntity = entity;
		}
		else if (pFlagSpawnComponents->HasComponent(entity))
		{
			TeamComponent teamComponent = {};
			if (pTeamComponents->GetConstIf(entity, teamComponent))
			{
				if (teamComponent.TeamIndex == m_LocalTeamIndex)
				{
					flagSpawnEntity = entity;
				}
			}
			else if (flagSpawnEntity == UINT32_MAX)
			{
				flagSpawnEntity = entity;
			}
		}
		else if (pPlayer)
		{
			if (pPlayer->GetTeam() == m_LocalTeamIndex)
				if (entity != localPlayer && !pPlayer->IsDead()) //remove local player from list
					teamPlayers.PushBack(entity);
		}
		else
		{
			mapSpectatePointEntity = entity;
		}

	}

	if (!m_IsGameOver)
	{
		if (!teamPlayers.IsEmpty()) //Spectate team-member
		{
			if (cameraEntity != UINT32_MAX)
			{
				ParentComponent& parentComponent = pParentComponents->GetData(cameraEntity);

				LOG_ERROR("TeamPlayer Size: %d", teamPlayers.GetSize());

				LOG_ERROR("index before modulu: %u", m_SpectatorIndex);
				m_SpectatorIndex = m_SpectatorIndex % teamPlayers.GetSize();
				LOG_ERROR("index after modulu: %u", m_SpectatorIndex);

				m_SpectatedPlayer = teamPlayers[m_SpectatorIndex];

				parentComponent.Parent = m_SpectatedPlayer;

				SpectatePlayerEvent event(PlayerManagerClient::GetPlayer(m_SpectatedPlayer)->GetName(), true);
				EventQueue::SendEventImmediate(event);
			}

		}
		else // spectate Flag
		{
			if (flagSpawnEntity != UINT32_MAX && cameraEntity != UINT32_MAX)
			{
				ParentComponent& parentComponent = pParentComponents->GetData(cameraEntity);
				parentComponent.Parent = flagSpawnEntity;
			}
		}
	}
	else if (m_IsGameOver)
	{
		if (cameraEntity != UINT32_MAX && mapSpectatePointEntity != UINT32_MAX)
		{
			ParentComponent& parentComponent = pParentComponents->GetData(cameraEntity);
			parentComponent.Parent = mapSpectatePointEntity;
		}
	}
}
