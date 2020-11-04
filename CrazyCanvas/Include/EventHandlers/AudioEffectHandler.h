#pragma once
#include "Physics/PhysicsEvents.h"

#include "Events/GameplayEvents.h"

#include "Application/API/Events/WindowEvents.h"

struct ProjectileHitEvent;

namespace LambdaEngine
{
	class ISoundEffect3D;
}

// AudioEffectHandler handles events that should cause audio effects to be played
class AudioEffectHandler
{
public:
	AudioEffectHandler() = default;
	~AudioEffectHandler();

	void Init();

private:
	bool OnPlayerDied(const PlayerDiedEvent& event);
	bool OnPlayerConnected(const PlayerConnectedEvent& event);
	bool OnPlayerHit(const PlayerHitEvent& event);
	bool OnWindowFocusChanged(const LambdaEngine::WindowFocusChangedEvent& event);

private:
	LambdaEngine::ISoundEffect3D* m_pEnemyHitSound		= nullptr;
	LambdaEngine::ISoundEffect3D* m_pHitSound			= nullptr;
	LambdaEngine::ISoundEffect3D* m_pPlayerKilledSound	= nullptr;
	LambdaEngine::ISoundEffect3D* m_pConnectSound		= nullptr;
	bool m_HasFocus;
};
