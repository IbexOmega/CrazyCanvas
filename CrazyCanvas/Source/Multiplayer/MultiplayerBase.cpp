#include "Multiplayer/MultiplayerBase.h"

#include "Multiplayer/Packet/PacketType.h"

#include "Match/Match.h"

#include "ECS/Systems/Player/WeaponSystem.h"

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
}

void MultiplayerBase::InitInternal()
{
	PacketType::Init();
	m_PacketDecoderSystem.Init();

	if (!Match::Init())
	{
		LOG_ERROR("Match Init Failed");
	}

	WeaponSystem::GetInstance().Init();
	Init();
}

void MultiplayerBase::TickMainThreadInternal(LambdaEngine::Timestamp deltaTime)
{
	TickMainThread(deltaTime);
	Match::Tick(deltaTime);
}

void MultiplayerBase::FixedTickMainThreadInternal(LambdaEngine::Timestamp deltaTime)
{
	FixedTickMainThread(deltaTime);
	WeaponSystem::GetInstance().FixedTick(deltaTime);

	// THIS SHOULD BE CALLED LAST DO NOT PLACE CODE BELOW
	m_PacketDecoderSystem.FixedTickMainThread(deltaTime);
}