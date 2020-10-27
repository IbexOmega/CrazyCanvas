#pragma once

#include "Application/API/Events/Event.h"

/*
* OnFlagDeliveredEvent
*/
struct OnFlagDeliveredEvent : public LambdaEngine::Event
{
public:
	inline OnFlagDeliveredEvent(uint32 teamIndex) : 
		Event(),
		TeamIndex(teamIndex)
	{
	}

	DECLARE_EVENT_TYPE(OnFlagDeliveredEvent);

	virtual LambdaEngine::String ToString() const override
	{
		return LambdaEngine::String("OnFlagDeliveredEvent");
	}

	uint8 TeamIndex = 0;
};