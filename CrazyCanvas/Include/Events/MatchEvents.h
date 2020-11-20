#pragma once

#include "Application/API/Events/Event.h"
#include "Match/MatchGameMode.h"

/*
* MatchInitializedEvent
*/
struct MatchInitializedEvent : public LambdaEngine::Event
{
public:
	inline MatchInitializedEvent(EGameMode gameMode)
		: Event()
		, GameMode(gameMode)
	{
	}

	DECLARE_EVENT_TYPE(MatchInitializedEvent);

	virtual LambdaEngine::String ToString() const override
	{
		return LambdaEngine::String("MatchInitializedEvent");
	}

	EGameMode GameMode = EGameMode::NONE;
};

/*
* MatchCountdownEvent
*/
struct MatchCountdownEvent : public LambdaEngine::Event
{
public:
	inline MatchCountdownEvent(uint8 countDownTime)
		: Event()
		, CountDownTime(countDownTime)
	{
	}

	DECLARE_EVENT_TYPE(MatchCountdownEvent);

	virtual LambdaEngine::String ToString() const override
	{
		return LambdaEngine::String("MatchCountdownEvent");
	}

	uint8 CountDownTime = 0;
};

/*
* FlagDeliveredEvent
*/
struct FlagDeliveredEvent : public LambdaEngine::Event
{
public:
	inline FlagDeliveredEvent(LambdaEngine::Entity entity, LambdaEngine::Entity entityPlayer, uint8 flagTeamIndex, uint8 scoringTeamIndex)
		: Event()
		, Entity(entity)
		, FlagTeamIndex(flagTeamIndex)
		, ScoringTeamIndex(scoringTeamIndex)
		, EntityPlayer(entityPlayer)
	{
	}

	DECLARE_EVENT_TYPE(FlagDeliveredEvent);

	virtual LambdaEngine::String ToString() const override
	{
		return LambdaEngine::String("FlagDeliveredEvent");
	}

	LambdaEngine::Entity Entity;
	LambdaEngine::Entity EntityPlayer;
	uint8 FlagTeamIndex = 0;
	uint8 ScoringTeamIndex = 0;
};

/*
* FlagRespawnEvent
*/
struct FlagRespawnEvent : public LambdaEngine::Event
{
public:
	inline FlagRespawnEvent(LambdaEngine::Entity entity, uint8 flagTeamIndex)
		: Event()
		, Entity(entity)
		, FlagTeamIndex(flagTeamIndex)
	{
	}

	DECLARE_EVENT_TYPE(FlagRespawnEvent);

	virtual LambdaEngine::String ToString() const override
	{
		return LambdaEngine::String("FlagRespawnEvent");
	}

	LambdaEngine::Entity Entity;
	uint8 FlagTeamIndex = 0;
};


struct GameOverEvent : public LambdaEngine::Event
{
public:
	inline GameOverEvent(uint8 teamIndex)
		: Event()
		, WinningTeamIndex(teamIndex)
	{
	}

	DECLARE_EVENT_TYPE(GameOverEvent);

	virtual LambdaEngine::String ToString() const override
	{
		return LambdaEngine::String("GameOverEvent");
	}

	uint8 WinningTeamIndex = 0;
};