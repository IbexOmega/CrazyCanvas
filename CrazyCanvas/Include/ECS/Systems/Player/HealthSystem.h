#pragma once
#include "ECS/System.h"

#include "Physics/PhysicsEvents.h"

#include "Threading/API/SpinLock.h"

/*
* HealthSystem
*/

class HealthSystem : public LambdaEngine::System
{
public:
	bool Init();

	virtual void Tick(LambdaEngine::Timestamp deltaTime) override final;

public:
	static HealthSystem& GetInstance() { return s_Instance; }

private:
	~HealthSystem() = default;
	HealthSystem() = default;
	
	bool OnProjectileHit(const ProjectileHitEvent& projectileHitEvent);

private:
	LambdaEngine::IDVector m_HealthEntities;
	LambdaEngine::SpinLock m_DeferredEventsLock;
	LambdaEngine::TArray<ProjectileHitEvent> m_DeferredHitEvents;
	LambdaEngine::TArray<ProjectileHitEvent> m_EventsToProcess;

private:
	static HealthSystem s_Instance;
};