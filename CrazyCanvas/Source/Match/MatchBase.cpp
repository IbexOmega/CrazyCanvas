#include "Match/MatchBase.h"

#include "World/LevelManager.h"
#include "World/Level.h"

#include "Application/API/Events/EventQueue.h"

#include "Events/GameplayEvents.h"

MatchBase::~MatchBase()
{
	using namespace LambdaEngine;
	EventQueue::UnregisterEventHandler<PacketReceivedEvent>(this, &MatchBase::OnPacketReceived);
	EventQueue::UnregisterEventHandler<WeaponFiredEvent>(this, &MatchBase::OnWeaponFired);

	SAFEDELETE(m_pLevel);
}

bool MatchBase::Init(const MatchDescription* pDesc)
{
	using namespace LambdaEngine;

	// Register eventhandlers
	EventQueue::RegisterEventHandler<PacketReceivedEvent>(this, &MatchBase::OnPacketReceived);
	EventQueue::RegisterEventHandler<WeaponFiredEvent>(this, &MatchBase::OnWeaponFired);
	
	m_pLevel = LevelManager::LoadLevel(pDesc->LevelHash);
	MultiplayerUtils::RegisterClientEntityAccessor(m_pLevel);

	if (m_pLevel == nullptr)
	{
		return false;
	}

	if (!InitInternal())
	{
		return false;
	}

	return true;
}

void MatchBase::Tick(LambdaEngine::Timestamp deltaTime)
{
	TickInternal(deltaTime);
}
