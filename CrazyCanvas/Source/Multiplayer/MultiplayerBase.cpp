#include "Multiplayer/MultiplayerBase.h"

#include "Multiplayer/Packet/PacketType.h"

#include "Match/Match.h"

#include "ECS/Systems/Player/WeaponSystem.h"
#include "ECS/Systems/Player/HealthSystem.h"

MultiplayerBase::MultiplayerBase() : 
	m_PacketDecoderSystem()
{

}

MultiplayerBase::~MultiplayerBase()
{
	if (!Match::Release())
	{
		LOG_ERROR("Match Release Failed");
	}
	PacketType::Release();
}

void MultiplayerBase::InitInternal()
{
	PacketType::Init();
	m_PacketDecoderSystem.Init();
	m_PlayerAnimationSystem.Init();

	if (!Match::Init())
	{
		LOG_ERROR("Match Init Failed");
	}

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
	HealthSystem::GetInstance().FixedTick(deltaTime);
	
	Match::FixedTick(deltaTime);

	FixedTickMainThread(deltaTime);

	WeaponSystem::GetInstance().FixedTick(deltaTime);

	PostFixedTickMainThread(deltaTime);
}
