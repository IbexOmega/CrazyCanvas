#pragma once
#include "HealthSystem.h"

#include "Threading/API/SpinLock.h"

/*
* HealthSystemServer
*/

class HealthSystemServer : public HealthSystem
{
	struct HitInfo
	{
		LambdaEngine::Entity Player;
		LambdaEngine::Entity ProjectileOwner;
	};

public:
	HealthSystemServer() = default;
	~HealthSystemServer();

	virtual void FixedTick(LambdaEngine::Timestamp deltaTime) override final;

protected:
	virtual bool InitInternal() override final;

	bool OnProjectileHit(const ProjectileHitEvent& projectileHitEvent);

	void InternalResetHealth(LambdaEngine::Entity entity);

public:
	static void ResetHealth(LambdaEngine::Entity entity);

private:
	LambdaEngine::IDVector m_MeshPaintEntities;

	LambdaEngine::SpinLock m_DeferredHitInfoLock;
	LambdaEngine::TArray<HitInfo> m_DeferredHitInfo;
	LambdaEngine::TArray<HitInfo> m_HitInfoToProcess;

	LambdaEngine::SpinLock m_DeferredResetsLock;
	LambdaEngine::TArray<LambdaEngine::Entity> m_DeferredResets;
	LambdaEngine::TArray<LambdaEngine::Entity> m_ResetsToProcess;
};