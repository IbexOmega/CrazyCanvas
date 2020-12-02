#pragma once
#include "Physics/PhysicsEvents.h"

#include "Events/PlayerEvents.h"
#include "Events/GameplayEvents.h"

#include "Application/API/Events/WindowEvents.h"

#include "Resources/ResourceCatalog.h"

struct ProjectileHitEvent;

namespace LambdaEngine
{
	class ISoundEffect2D;
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
	bool OnPlayerAliveUpdatedEvent(const PlayerAliveUpdatedEvent& event);
	bool OnPlayerHit(const PlayerHitEvent& event);
	bool OnWindowFocusChanged(const LambdaEngine::WindowFocusChangedEvent& event);

private:
	// 3D sounds
	LambdaEngine::ISoundEffect3D* m_pEnemyHitSound		= nullptr;
	LambdaEngine::ISoundEffect3D* m_pPlayerKilledSound	= nullptr;
	
	// 2D sounds
	LambdaEngine::ISoundEffect2D* m_pHitSound			= nullptr;
	
	bool m_HasFocus = false;
};
