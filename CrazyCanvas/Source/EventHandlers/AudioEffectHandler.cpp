#include "EventHandlers/AudioEffectHandler.h"

#include "Application/API/Events/EventQueue.h"
#include "Application/API/CommonApplication.h"

#include "Audio/API/ISoundEffect3D.h"

#include "RenderStages/PaintMaskRenderer.h"

#include "Game/Multiplayer/MultiplayerUtils.h"

#include "ECS/ECSCore.h"

using namespace LambdaEngine;

AudioEffectHandler::~AudioEffectHandler()
{
	if (!MultiplayerUtils::IsServer())
	{
		EventQueue::UnregisterEventHandler(this, &AudioEffectHandler::OnPlayerAliveUpdatedEvent);
		EventQueue::UnregisterEventHandler(this, &AudioEffectHandler::OnPlayerConnected);
		EventQueue::UnregisterEventHandler(this, &AudioEffectHandler::OnPlayerHit);
		EventQueue::UnregisterEventHandler(this, &AudioEffectHandler::OnWindowFocusChanged);
	}
}

void AudioEffectHandler::Init()
{
	if (!MultiplayerUtils::IsServer())
	{
		// Load resources
		{
			// Load 3D sounds
			GUID_Lambda killedSoundID = ResourceManager::LoadSoundEffect3DFromFile("Player/DeathSound.mp3");
			m_pPlayerKilledSound = ResourceManager::GetSoundEffect3D(killedSoundID);

			GUID_Lambda enemyhitSoundID = ResourceManager::LoadSoundEffect3DFromFile("Player/HitSound0.mp3");
			m_pEnemyHitSound = ResourceManager::GetSoundEffect3D(enemyhitSoundID);

			// Load 2D sounds
			GUID_Lambda connectSoundID = ResourceManager::LoadSoundEffect2DFromFile("connect_sound.mp3");
			m_pConnectSound = ResourceManager::GetSoundEffect2D(connectSoundID);

			GUID_Lambda hitSoundID = ResourceManager::LoadSoundEffect2DFromFile("Player/HitSound1.mp3");
			m_pHitSound = ResourceManager::GetSoundEffect2D(hitSoundID);
		}
		// Register callbacks
		EventQueue::RegisterEventHandler(this, &AudioEffectHandler::OnPlayerAliveUpdatedEvent);
		EventQueue::RegisterEventHandler(this, &AudioEffectHandler::OnPlayerConnected);
		EventQueue::RegisterEventHandler(this, &AudioEffectHandler::OnPlayerHit);
		EventQueue::RegisterEventHandler(this, &AudioEffectHandler::OnWindowFocusChanged);

		// Retrive current state of window
		TSharedRef<Window> mainWindow = CommonApplication::Get()->GetMainWindow();
		m_HasFocus = mainWindow->IsActiveWindow();
	}
}

bool AudioEffectHandler::OnPlayerAliveUpdatedEvent(const PlayerAliveUpdatedEvent& event)
{
	const Player* pPlayer = event.pPlayer;
	const ECSCore* pECSCore = ECSCore::GetInstance();

	if (m_HasFocus)
	{
		if (pPlayer->IsDead())
		{
			const auto* pPositionComponents = pECSCore->GetComponentArray<PositionComponent>();
			if (pPositionComponents->HasComponent(pPlayer->GetEntity()))
			{
				const PositionComponent& positionComponent = pPositionComponents->GetConstData(pPlayer->GetEntity());

				m_pPlayerKilledSound->PlayOnceAt(positionComponent.Position, glm::vec3(0.0f), 0.5f);
			}
		}
		return true;
	}
	else
	{
		return false;
	}
}

bool AudioEffectHandler::OnPlayerConnected(const PlayerConnectedEvent& event)
{
	UNREFERENCED_VARIABLE(event);

	if (m_HasFocus)
	{
		m_pConnectSound->PlayOnce();
		return true;
	}
	else
	{
		return false;
	}
}

bool AudioEffectHandler::OnPlayerHit(const PlayerHitEvent& event)
{
	if (m_HasFocus)
	{
		if (event.IsLocal)
		{
			m_pHitSound->PlayOnce(0.5f);
		}
		else
		{
			m_pEnemyHitSound->PlayOnceAt(event.HitPosition, glm::vec3(0.0f), 0.5f);
		}

		return true;
	}
	else
	{
		return false;
	}
}

bool AudioEffectHandler::OnWindowFocusChanged(const LambdaEngine::WindowFocusChangedEvent& event)
{
	m_HasFocus = event.HasFocus;
	return true;
}
