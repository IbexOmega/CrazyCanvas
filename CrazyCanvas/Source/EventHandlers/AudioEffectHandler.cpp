#include "EventHandlers/AudioEffectHandler.h"

#include "Application/API/Events/EventQueue.h"

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
	}
}

bool AudioEffectHandler::OnPlayerDied(const PlayerDiedEvent& event)
{
	UNREFERENCED_VARIABLE(event);

	m_pPlayerKilledSound->PlayOnce();
	return true;
}

bool AudioEffectHandler::OnPlayerConnected(const PlayerConnectedEvent& event)
{
	UNREFERENCED_VARIABLE(event);

	m_pConnectSound->PlayOnce();
	return true;
}

bool AudioEffectHandler::OnPlayerHit(const PlayerHitEvent& event)
{
	if (event.IsLocal)
	{
		m_pHitSound->PlayOnce(0.5f);
	}
	else
	{
		m_pEnemyHitSound->PlayOnce(0.5f);
	}

	return false;
}
