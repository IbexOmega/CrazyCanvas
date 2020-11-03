#pragma once

#include "Application/API/Events/Event.h"

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