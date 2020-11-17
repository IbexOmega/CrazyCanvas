#pragma once

#include "Match/MatchBase.h"

#include "Time/API/Timestamp.h"

class Match
{
public:
	DECL_STATIC_CLASS(Match);

	static bool Init();
	static bool Release();

	static bool CreateMatch(const MatchDescription* pDesc);
	static bool ResetMatch();
	static bool ReleaseMatch();

	static void StartMatch();
	static void BeginLoading();

	static void Tick(LambdaEngine::Timestamp deltaTime);
	static void FixedTick(LambdaEngine::Timestamp deltaTime);

	FORCEINLINE static bool HasBegun()
	{ 
		return s_pMatchInstance != nullptr ? s_pMatchInstance->HasBegun() : false; 
	}

	FORCEINLINE static uint32 GetScore(uint32 teamIndex)
	{ 
		return s_pMatchInstance != nullptr ? s_pMatchInstance->GetScore(teamIndex) : 0; 
	}

	FORCEINLINE static MatchBase* GetInstance()
	{
		VALIDATE(s_pMatchInstance != nullptr);
		return s_pMatchInstance;
	}

private:
	inline static MatchBase* s_pMatchInstance = nullptr;
};
