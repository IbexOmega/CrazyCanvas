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

	void ResetEntityHealth(LambdaEngine::Entity entityToReset);

	void FixedTick(LambdaEngine::Timestamp deltaTime);

	virtual void Tick(LambdaEngine::Timestamp deltaTime) override final
	{
		UNREFERENCED_VARIABLE(deltaTime);
	}

public:
	static HealthSystem& GetInstance() { return s_Instance; }

private:
	HealthSystem()	= default;
	~HealthSystem()	= default;
	
	bool OnProjectileHit(const ProjectileHitEvent& projectileHitEvent);

private:
	LambdaEngine::IDVector m_HealthEntities;
	LambdaEngine::IDVector m_LocalPlayerEntities;

	LambdaEngine::SpinLock m_DeferredEventsLock;
	LambdaEngine::TArray<ProjectileHitEvent> m_DeferredHitEvents;
	LambdaEngine::TArray<ProjectileHitEvent> m_EventsToProcess;

	LambdaEngine::SpinLock m_DeferredResetsLock;
	LambdaEngine::TArray<LambdaEngine::Entity> m_DeferredResets;

private:
	static HealthSystem s_Instance;
};