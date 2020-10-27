#include "Multiplayer/MultiplayerBase.h"

#include "Multiplayer/Packet/PacketType.h"

#include "Match/Match.h"

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

	if (!Match::Init())
	{
		LOG_ERROR("Match Init Failed");
	}

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
	m_PacketDecoderSystem.FixedTickMainThread(deltaTime);
}