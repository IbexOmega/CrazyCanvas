#pragma once
#include "Physics/PhysicsEvents.h"

#include "Events/GameplayEvents.h"

struct ProjectileHitEvent;

namespace LambdaEngine
{
	class ISoundEffect2D;
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

private:
	LambdaEngine::ISoundEffect2D* m_pEnemyHitSound		= nullptr;
	LambdaEngine::ISoundEffect2D* m_pHitSound			= nullptr;
	LambdaEngine::ISoundEffect2D* m_pPlayerKilledSound	= nullptr;
	LambdaEngine::ISoundEffect2D* m_pConnectSound		= nullptr;
};
