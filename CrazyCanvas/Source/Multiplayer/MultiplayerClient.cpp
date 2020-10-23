#include "Multiplayer/MultiplayerClient.h"

MultiplayerClient::MultiplayerClient() : 
	m_PlayerLocal(),
	m_PlayerForeignSystem()
{

}

MultiplayerClient::~MultiplayerClient()
{
	SAFEDELETE(m_pFlagSystem);
}

void MultiplayerClient::Init()
{
	m_PlayerLocal.Init();
	m_PlayerForeignSystem.Init();

	m_pFlagSystem = DBG_NEW ClientFlagSystem();
	m_pFlagSystem->Init();

	m_WeaponSystem.Init();
}

void MultiplayerClient::TickMainThread(LambdaEngine::Timestamp deltaTime)
{
	m_PlayerLocal.TickMainThread(deltaTime);
}

void MultiplayerClient::FixedTickMainThread(LambdaEngine::Timestamp deltaTime)
{
	m_PlayerLocal.FixedTickMainThread(deltaTime);
	m_PlayerForeignSystem.FixedTickMainThread(deltaTime);
	m_pFlagSystem->FixedTick(deltaTime);
	m_WeaponSystem.FixedTick(deltaTime);
}
