#include "Multiplayer/MultiplayerServer.h"

#include "ECS/Systems/Multiplayer/PacketTranscoderSystem.h"

MultiplayerServer::MultiplayerServer() :
	m_PlayerRemoteSystem(),
	m_pFlagSystem(nullptr),
	m_pShowerSystem(nullptr)
{
}

MultiplayerServer::~MultiplayerServer()
{
	SAFEDELETE(m_pFlagSystem);
	SAFEDELETE(m_pShowerSystem);
}

void MultiplayerServer::Init()
{
	m_pFlagSystem = DBG_NEW ServerFlagSystem();
	m_pFlagSystem->Init();
	m_pShowerSystem = DBG_NEW ServerShowerSystem();
	m_pShowerSystem->Init();
	m_PlayerRemoteSystem.Init();
}

void MultiplayerServer::TickMainThread(LambdaEngine::Timestamp deltaTime)
{
	UNREFERENCED_VARIABLE(deltaTime);
}

void MultiplayerServer::FixedTickMainThread(LambdaEngine::Timestamp deltaTime)
{
	m_pFlagSystem->FixedTick(deltaTime);
	m_pShowerSystem->FixedTick(deltaTime);
	m_PlayerRemoteSystem.FixedTickMainThread(deltaTime);
}

void MultiplayerServer::PostFixedTickMainThread(LambdaEngine::Timestamp deltaTime)
{
	//Must run last
	PacketTranscoderSystem::GetInstance().FixedTickMainThreadServer(deltaTime);
}
