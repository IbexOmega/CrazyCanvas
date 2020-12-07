#include "Multiplayer/MultiplayerClient.h"

#include "ECS/Systems/Multiplayer/PacketTranscoderSystem.h"

#include "Debug/Profiler.h"

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
	SAFEDELETE(m_pShowerSystem);
}

void MultiplayerClient::Init()
{
	m_ReplaySystem.Init();
	m_PlayerLocal.Init();

	m_PlayerForeignSystem.Init();
	m_NetworkPositionSystem.Init();

	m_pFlagSystem = DBG_NEW ClientFlagSystem();
	m_pFlagSystem->Init();

	m_pShowerSystem = DBG_NEW ClientShowerSystem();
	m_pShowerSystem->Init();
}

void MultiplayerClient::TickMainThread(LambdaEngine::Timestamp deltaTime)
{
	m_PlayerLocal.TickMainThread(deltaTime);
}

void MultiplayerClient::FixedTickMainThread(LambdaEngine::Timestamp deltaTime)
{
	PROFILE_FUNCTION("m_PlayerForeignSystem->FixedTickMainThread", m_PlayerForeignSystem.FixedTickMainThread(deltaTime));

	PROFILE_FUNCTION("m_ReplaySystem->FixedTickMainThread", m_ReplaySystem.FixedTickMainThread(deltaTime));

	PROFILE_FUNCTION("m_pFlagSystem->FixedTick", m_pFlagSystem->FixedTick(deltaTime));

	PROFILE_FUNCTION("m_pShowerSystem->FixedTick", m_pShowerSystem->FixedTick(deltaTime));
}

void MultiplayerClient::PostFixedTickMainThread(LambdaEngine::Timestamp deltaTime)
{
	// Must run last
	PacketTranscoderSystem::GetInstance().FixedTickMainThreadClient(deltaTime);
}
