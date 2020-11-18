#include "Multiplayer/MultiplayerClient.h"

#include "ECS/Systems/Multiplayer/PacketTranscoderSystem.h"

MultiplayerClient::MultiplayerClient() :
	m_PlayerLocal(),
	m_PlayerForeignSystem(),
	m_NetworkPositionSystem(),
	m_ReplaySystem()
{
}

MultiplayerClient::~MultiplayerClient()
{
	SAFEDELETE(m_pFlagSystem);
}

void MultiplayerClient::Init()
{
	m_ReplaySystem.Init();
	m_PlayerLocal.Init();

	m_PlayerForeignSystem.Init();
	m_NetworkPositionSystem.Init();

	m_pFlagSystem = DBG_NEW ClientFlagSystem();
	m_pFlagSystem->Init();
}

void MultiplayerClient::TickMainThread(LambdaEngine::Timestamp deltaTime)
{
	m_PlayerLocal.TickMainThread(deltaTime);
}

void MultiplayerClient::FixedTickMainThread(LambdaEngine::Timestamp deltaTime)
{
	m_PlayerForeignSystem.FixedTickMainThread(deltaTime);

	m_ReplaySystem.FixedTickMainThread(deltaTime);

	m_pFlagSystem->FixedTick(deltaTime);
}

void MultiplayerClient::PostFixedTickMainThread(LambdaEngine::Timestamp deltaTime)
{
	// Must run last
	PacketTranscoderSystem::GetInstance().FixedTickMainThreadClient(deltaTime);
}
