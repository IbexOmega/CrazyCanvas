#pragma once
#include "HealthSystem.h"

#include "Threading/API/SpinLock.h"

/*
* HealthSystemServer 
*/

class HealthSystemServer : public HealthSystem
{
public:
	HealthSystemServer() = default;
	~HealthSystemServer();

	virtual void FixedTick(LambdaEngine::Timestamp deltaTime) override final;

protected:
	virtual bool InitInternal() override final;

	bool OnProjectileHit(const ProjectileHitEvent& projectileHitEvent);

private:
	LambdaEngine::IDVector m_MeshPaintEntities;

	LambdaEngine::SpinLock m_DeferredEventsLock;
	LambdaEngine::TArray<ProjectileHitEvent> m_DeferredHitEvents;
	LambdaEngine::TArray<ProjectileHitEvent> m_EventsToProcess;
};