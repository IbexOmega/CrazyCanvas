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
	m_PlayerLocal.Init();
	m_PlayerForeignSystem.Init();
}

void MultiplayerClient::TickMainThread(LambdaEngine::Timestamp deltaTime)
{
	m_PlayerLocal.TickMainThread(deltaTime);
}

void MultiplayerClient::FixedTickMainThread(LambdaEngine::Timestamp deltaTime)
{
	m_PlayerLocal.FixedTickMainThread(deltaTime);
	m_PlayerForeignSystem.FixedTickMainThread(deltaTime);
}
