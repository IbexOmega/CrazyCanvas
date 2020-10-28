#pragma once
#include "LambdaEngine.h"

#include "Utilities/SHA256.h"

#include "Containers/String.h"

#include "Time/API/Timestamp.h"

#include "Application/API/Events/NetworkEvents.h"

#include "Events/GameplayEvents.h"

class Level;

struct CreateFlagDesc;
struct CreatePlayerDesc;

struct MatchDescription
{
	LambdaEngine::SHA256Hash LevelHash;
	uint32 NumTeams = 2;
};

class MatchBase
{
public:
	MatchBase() = default;
	virtual ~MatchBase();

	bool Init(const MatchDescription* pDesc);

	void Tick(LambdaEngine::Timestamp deltaTime);

	void SetScore(uint32 teamIndex, uint32 score);

	FORCEINLINE uint32 GetScore(uint32 teamIndex) const { VALIDATE(teamIndex < m_Scores.GetSize()); return m_Scores[teamIndex]; }

protected:
	virtual bool InitInternal() = 0;
	virtual void TickInternal(LambdaEngine::Timestamp deltaTime) = 0;

	virtual bool OnPacketReceived(const LambdaEngine::NetworkSegmentReceivedEvent& event) = 0;
	virtual bool OnWeaponFired(const WeaponFiredEvent& event) = 0;
	virtual bool OnPlayerDied(const PlayerDiedEvent& event) = 0;
	
protected:
	Level* m_pLevel = nullptr;
	LambdaEngine::TArray<uint32> m_Scores;
};