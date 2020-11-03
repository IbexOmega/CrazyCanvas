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
		GUID_Lambda killedSoundID = ResourceManager::LoadSoundEffectFromFile("death_sound.mp3");
		m_pPlayerKilledSound = ResourceManager::GetSoundEffect(killedSoundID);

		GUID_Lambda connectSoundID = ResourceManager::LoadSoundEffectFromFile("connect_sound.mp3");
		m_pConnectSound = ResourceManager::GetSoundEffect(connectSoundID);

		GUID_Lambda hitSoundID = ResourceManager::LoadSoundEffectFromFile("hit_sound.mp3");
		m_pHitSound = ResourceManager::GetSoundEffect(hitSoundID);

		GUID_Lambda enemyhitSoundID = ResourceManager::LoadSoundEffectFromFile("enemy_hit_sound.mp3");
		m_pEnemyHitSound = ResourceManager::GetSoundEffect(enemyhitSoundID);

		// Register callbacks
		EventQueue::RegisterEventHandler(this, &AudioEffectHandler::OnPlayerDied);
		EventQueue::RegisterEventHandler(this, &AudioEffectHandler::OnPlayerConnected);
		EventQueue::RegisterEventHandler(this, &AudioEffectHandler::OnPlayerHit);
	}
}

bool AudioEffectHandler::OnPlayerDied(const PlayerDiedEvent& event)
{
	m_pPlayerKilledSound->PlayOnceAt(event.Position);
	return true;
}

bool AudioEffectHandler::OnPlayerConnected(const PlayerConnectedEvent& event)
{
	m_pConnectSound->PlayOnceAt(event.Position);
	return true;
}

bool AudioEffectHandler::OnPlayerHit(const PlayerHitEvent& event)
{
	if (event.IsLocal)
	{
		m_pHitSound->PlayOnceAt(event.HitPosition);
	}
	else
	{
		m_pEnemyHitSound->PlayOnceAt(event.HitPosition);
	}

	return false;
}
