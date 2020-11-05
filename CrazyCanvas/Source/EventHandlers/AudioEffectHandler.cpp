#include "EventHandlers/AudioEffectHandler.h"

#include "Application/API/Events/EventQueue.h"
#include "Application/API/CommonApplication.h"

#include "Audio/API/ISoundEffect3D.h"

#include "Rendering/PaintMaskRenderer.h"

#include "Game/Multiplayer/MultiplayerUtils.h"

AudioEffectHandler::~AudioEffectHandler()
{
	using namespace LambdaEngine;
	
	if (!MultiplayerUtils::IsServer())
	{
		EventQueue::UnregisterEventHandler(this, &AudioEffectHandler::OnPlayerDied);
		EventQueue::UnregisterEventHandler(this, &AudioEffectHandler::OnPlayerConnected);
		EventQueue::UnregisterEventHandler(this, &AudioEffectHandler::OnPlayerHit);
		EventQueue::UnregisterEventHandler(this, &AudioEffectHandler::OnWindowFocusChanged);
	}
}

void AudioEffectHandler::Init()
{
	using namespace LambdaEngine;

	if (!MultiplayerUtils::IsServer())
	{
		// Load resources
		GUID_Lambda killedSoundID = ResourceManager::LoadSoundEffect2DFromFile("death_sound.mp3");
		m_pPlayerKilledSound = ResourceManager::GetSoundEffect2D(killedSoundID);

		GUID_Lambda connectSoundID = ResourceManager::LoadSoundEffect2DFromFile("connect_sound.mp3");
		m_pConnectSound = ResourceManager::GetSoundEffect2D(connectSoundID);

		GUID_Lambda hitSoundID = ResourceManager::LoadSoundEffect2DFromFile("hit_sound.mp3");
		m_pHitSound = ResourceManager::GetSoundEffect2D(hitSoundID);

		GUID_Lambda enemyhitSoundID = ResourceManager::LoadSoundEffect2DFromFile("enemy_hit_sound.mp3");
		m_pEnemyHitSound = ResourceManager::GetSoundEffect2D(enemyhitSoundID);

		// Register callbacks
		EventQueue::RegisterEventHandler(this, &AudioEffectHandler::OnPlayerDied);
		EventQueue::RegisterEventHandler(this, &AudioEffectHandler::OnPlayerConnected);
		EventQueue::RegisterEventHandler(this, &AudioEffectHandler::OnPlayerHit);
		EventQueue::RegisterEventHandler(this, &AudioEffectHandler::OnWindowFocusChanged);

		// Retrive current state of window
		TSharedRef<Window> mainWindow = CommonApplication::Get()->GetMainWindow();
		m_HasFocus = mainWindow->IsActiveWindow();
	}
}

bool AudioEffectHandler::OnPlayerDied(const PlayerDiedEvent& event)
{
	if (m_HasFocus)
	{
		m_pPlayerKilledSound->PlayOnceAt(event.Position);
		return true;
	}
	else
	{
		return false;
	}
}

bool AudioEffectHandler::OnPlayerConnected(const PlayerConnectedEvent& event)
{
	if (m_HasFocus)
	{
		m_pConnectSound->PlayOnceAt(event.Position);
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
			m_pHitSound->PlayOnceAt(event.HitPosition, glm::vec3(0.0f), 0.5f);
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
