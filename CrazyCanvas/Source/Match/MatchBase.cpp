#include "Match/MatchBase.h"

#include "World/LevelManager.h"
#include "World/Level.h"

#include "Application/API/Events/EventQueue.h"

MatchBase::~MatchBase()
{
	SAFEDELETE(m_pLevel);
}

bool MatchBase::Init(const MatchDescription* pDesc)
{
	using namespace LambdaEngine;

	m_pLevel = LevelManager::LoadLevel(pDesc->LevelHash);
	m_MatchDesc = *pDesc;

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

void MatchBase::SetScore(uint32 teamIndex, uint32 score)
{
	VALIDATE(teamIndex < m_Scores.GetSize());
	m_Scores[teamIndex] = score;
}

void MatchBase::ResetMatch()
{
	for (uint32 i = 0; i < m_MatchDesc.NumTeams; i++)
	{
		SetScore(i, 0);
	}
}
