#pragma once

#include "Physics/PhysicsEvents.h"

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
	bool OnProjectileHit(const ProjectileHitEvent& projectileHitEvent);

private:
	LambdaEngine::ISoundEffect3D* m_pProjectileHitSound = nullptr;
};
