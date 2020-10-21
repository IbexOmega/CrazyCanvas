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

	static HealthSystem& GetInstance();

private:
	HealthSystem();
	~HealthSystem();

	bool OnProjectileHit(const ProjectileHitEvent& projectileHitEvent);

private:
	LambdaEngine::IDVector m_HealthEntities;
	LambdaEngine::SpinLock m_DefferedEventsLock;
	LambdaEngine::TArray<ProjectileHitEvent> m_DeferredHitEvents;
	LambdaEngine::TArray<ProjectileHitEvent> m_EventsToProcess;
};