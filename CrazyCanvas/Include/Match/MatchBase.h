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
	uint32 MaxScore = 2;
};

static constexpr const float32 MATCH_BEGIN_COUNTDOWN_TIME = 5.0f;

class MatchBase
{
public:
	MatchBase() = default;
	virtual ~MatchBase();

	bool Init(const MatchDescription* pDesc);

	void Tick(LambdaEngine::Timestamp deltaTime);
	void FixedTick(LambdaEngine::Timestamp deltaTime);

	void SetScore(uint32 teamIndex, uint32 score);

	void ResetMatch();

	virtual void KillPlayer(LambdaEngine::Entity entityToKill, LambdaEngine::Entity killedByEntity)
	{
		UNREFERENCED_VARIABLE(entityToKill);
		UNREFERENCED_VARIABLE(killedByEntity);
	}

	virtual void BeginLoading()
	{

	}

	virtual void MatchStart()
	{

	}

	FORCEINLINE bool HasBegun() const { return m_HasBegun; }
	FORCEINLINE uint32 GetScore(uint32 teamIndex) const { VALIDATE(teamIndex < m_Scores.GetSize()); return m_Scores[teamIndex]; }

protected:
	virtual bool InitInternal() = 0;
	virtual void TickInternal(LambdaEngine::Timestamp deltaTime) = 0;

	virtual void FixedTickInternal(LambdaEngine::Timestamp deltaTime)
	{
		UNREFERENCED_VARIABLE(deltaTime);
	}

	virtual bool OnWeaponFired(const WeaponFiredEvent& event) = 0;
	
protected:
	Level* m_pLevel = nullptr;
	LambdaEngine::TArray<uint32> m_Scores;

	MatchDescription m_MatchDesc;

	bool m_HasBegun = false;
	float32 m_MatchBeginTimer = 0.0f;
};