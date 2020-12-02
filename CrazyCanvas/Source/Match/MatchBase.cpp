#include "Match/MatchBase.h"

#include "World/LevelManager.h"
#include "World/Level.h"

#include "Application/API/Events/EventQueue.h"

#include "Events/GameplayEvents.h"

MatchBase::MatchBase()
{
	using namespace LambdaEngine;
	EventQueue::RegisterEventHandler<WeaponFiredEvent>(this, &MatchBase::OnWeaponFired);
}

MatchBase::~MatchBase()
{
	using namespace LambdaEngine;

	EventQueue::UnregisterEventHandler<WeaponFiredEvent>(this, &MatchBase::OnWeaponFired);

	SAFEDELETE(m_pLevel);
}

bool MatchBase::Init(const MatchDescription* pDesc)
{
	using namespace LambdaEngine;

	m_pLevel = LevelManager::LoadLevel(pDesc->LevelHash);
	m_MatchDesc = *pDesc;

	m_GameModeHasFlag = GameModeRequiresFlag(m_MatchDesc.GameMode);

	if (m_pLevel == nullptr)
	{
		return false;
	}

	if (!InitInternal())
	{
		return false;
	}

	m_Scores.Resize(m_MatchDesc.NumTeams);

	return true;
}

void MatchBase::Tick(LambdaEngine::Timestamp deltaTime)
{
	TickInternal(deltaTime);
}

void MatchBase::FixedTick(LambdaEngine::Timestamp deltaTime)
{
	FixedTickInternal(deltaTime);
}

bool MatchBase::SetScore(uint8 teamIndex, uint32 score)
{
	uint32 index = teamIndex - 1;
	VALIDATE(index < m_Scores.GetSize());
	if (m_Scores[index] != score)
	{
		m_Scores[index] = score;
		return true;
	}
	return false;
}

void MatchBase::ResetMatch()
{
	for (uint8 i = 0; i < m_Scores.GetSize(); i++)
	{
		m_Scores[i] = 0;
	}

	SAFEDELETE(m_pLevel);

	m_GameModeHasFlag = false;
	m_HasBegun = false;
	m_MatchBeginTimer = 0.0f;

	ResetMatchInternal();
}