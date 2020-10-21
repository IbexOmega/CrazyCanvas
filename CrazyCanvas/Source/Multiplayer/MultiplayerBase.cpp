#include "Multiplayer/MultiplayerBase.h"

#include "Multiplayer/PacketType.h"

MultiplayerBase::MultiplayerBase() : 
	m_PacketDecoderSystem()
{

}

void MultiplayerBase::InitInternal()
{
	PacketType::Init();
	m_PacketDecoderSystem.Init();
	Init();
}

void MultiplayerBase::TickMainThreadInternal(LambdaEngine::Timestamp deltaTime)
{
	TickMainThread(deltaTime);
}

void MultiplayerBase::FixedTickMainThreadInternal(LambdaEngine::Timestamp deltaTime)
{
	FixedTickMainThread(deltaTime);
	m_PacketDecoderSystem.FixedTickMainThread(deltaTime);
}