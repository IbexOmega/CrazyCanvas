#include "Multiplayer/MultiplayerClient.h"

MultiplayerClient::MultiplayerClient() : 
	m_PlayerLocal(),
	m_PlayerForeignSystem()
{

}

MultiplayerClient::~MultiplayerClient()
{

}

void MultiplayerClient::Init()
{
	MultiplayerBase::Init();
	m_PlayerLocal.Init();
	m_PlayerForeignSystem.Init();
}

void MultiplayerClient::TickMainThread(LambdaEngine::Timestamp deltaTime)
{
	MultiplayerBase::TickMainThread(deltaTime);
	m_PlayerLocal.TickMainThread(deltaTime);
}

void MultiplayerClient::FixedTickMainThread(LambdaEngine::Timestamp deltaTime)
{
	MultiplayerBase::FixedTickMainThread(deltaTime);
	m_PlayerLocal.FixedTickMainThread(deltaTime);
	m_PlayerForeignSystem.FixedTickMainThread(deltaTime);
}
