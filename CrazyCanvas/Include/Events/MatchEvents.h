#pragma once

#include "Application/API/Events/Event.h"

/*
* FlagDeliveredEvent
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
	inline FlagDeliveredEvent(uint8 teamIndex)
		: Event()
		, TeamIndex(teamIndex)
	{
	}

	DECLARE_EVENT_TYPE(FlagDeliveredEvent);

	virtual LambdaEngine::String ToString() const override
	{
		return LambdaEngine::String("FlagDeliveredEvent");
	}

	uint8 TeamIndex = 0;
};