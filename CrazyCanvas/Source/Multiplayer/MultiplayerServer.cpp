#include "Multiplayer/MultiplayerServer.h"

MultiplayerServer::MultiplayerServer()
{
}

MultiplayerServer::~MultiplayerServer()
{
	SAFEDELETE(m_pFlagSystem);
}

void MultiplayerServer::Init()
{
	m_pFlagSystem = DBG_NEW ServerFlagSystem();
	m_pFlagSystem->Init();

	m_WeaponSystem.Init();
}

void MultiplayerServer::TickMainThread(LambdaEngine::Timestamp deltaTime)
{
}

void MultiplayerServer::FixedTickMainThread(LambdaEngine::Timestamp deltaTime)
{
	m_pFlagSystem->FixedTick(deltaTime);
	m_WeaponSystem.FixedTick(deltaTime);
}
