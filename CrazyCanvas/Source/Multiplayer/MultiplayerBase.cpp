#include "Multiplayer/MultiplayerBase.h"

MultiplayerBase::MultiplayerBase() : 
	m_PacketDecoderSystem()
{

}

void MultiplayerBase::Init()
{
	m_PacketDecoderSystem.Init();
}

void MultiplayerBase::TickMainThread(LambdaEngine::Timestamp deltaTime)
{

}

void MultiplayerBase::FixedTickMainThread(LambdaEngine::Timestamp deltaTime)
{

}
