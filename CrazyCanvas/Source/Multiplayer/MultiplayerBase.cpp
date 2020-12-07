#include "Multiplayer/MultiplayerBase.h"

#include "Match/Match.h"

#include "ECS/Systems/Player/WeaponSystem.h"
#include "ECS/Systems/Player/HealthSystem.h"

#include "Debug/Profiler.h"

MultiplayerBase::MultiplayerBase()
{

}

MultiplayerBase::~MultiplayerBase()
{
	if (!Match::Release())
	{
		LOG_ERROR("Match Release Failed");
	}

	WeaponSystem::Release();
	HealthSystem::Release();
}

void MultiplayerBase::InitInternal()
{
	m_PlayerAnimationSystem.Init();

	WeaponSystem::Init();
	HealthSystem::Init();

	Init();
}

void MultiplayerBase::TickMainThreadInternal(LambdaEngine::Timestamp deltaTime)
{
	TickMainThread(deltaTime);
	Match::Tick(deltaTime);
}

void MultiplayerBase::FixedTickMainThreadInternal(LambdaEngine::Timestamp deltaTime)
{
	PROFILE_FUNCTION("HealthSystem.FixedTick", HealthSystem::GetInstance().FixedTick(deltaTime));
	PROFILE_FUNCTION("Match::FixedTick", Match::FixedTick(deltaTime));
	PROFILE_FUNCTION("FixedTickMainThread", FixedTickMainThread(deltaTime));
	PROFILE_FUNCTION("WeaponSystem::FixedTick", WeaponSystem::GetInstance().FixedTick(deltaTime));
	PROFILE_FUNCTION("PostFixedTickMainThread", PostFixedTickMainThread(deltaTime));
}
