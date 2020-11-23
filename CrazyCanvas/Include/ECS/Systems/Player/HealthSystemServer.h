#pragma once
#include "HealthSystem.h"

#include "Threading/API/SpinLock.h"

/*
* HealthSystemServer 
*/

namespace LambdaEngine
{
	class CommandAllocator;
	class CommandList;
	class Buffer;
	class Fence;
}

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
	bool CreateResources();

private:
	LambdaEngine::IDVector m_MeshPaintEntities;

	LambdaEngine::SpinLock m_DeferredHitInfoLock;
	LambdaEngine::TArray<HitInfo> m_DeferredHitInfo;
	LambdaEngine::TArray<HitInfo> m_HitInfoToProcess;

	LambdaEngine::SpinLock m_DeferredResetsLock;
	LambdaEngine::TArray<LambdaEngine::Entity> m_DeferredResets;
	LambdaEngine::TArray<LambdaEngine::Entity> m_ResetsToProcess;

	LambdaEngine::CommandAllocator*	m_CommandAllocator		= nullptr;
	LambdaEngine::CommandList*		m_CommandList			= nullptr;
	LambdaEngine::Buffer*			m_HealthBuffer			= nullptr;
	LambdaEngine::Buffer*			m_CopyBuffer			= nullptr;
	LambdaEngine::Buffer*			m_VertexCountBuffer		= nullptr;
	LambdaEngine::Fence*			m_CopyFence				= nullptr;
	uint64							m_FenceCounter			= 1;

	uint64 m_PlayerHealths[10];
	uint32 m_VertexCount = 0;
};