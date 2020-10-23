#pragma once
#include "Application/API/Events/Event.h"

#include "ECS/Entity.h"

/*
* PlayerDiedEvent
*/

struct PlayerDiedEvent : public LambdaEngine::Event
{
public:
	inline PlayerDiedEvent(const LambdaEngine::Entity killedEntity)
		: Event()
		, KilledEntity(killedEntity)
	{
	}

	DECLARE_EVENT_TYPE(PlayerDiedEvent);

	virtual LambdaEngine::String ToString() const
	{
		using namespace LambdaEngine;
		return String("Player Died. EntitiyID=%u", KilledEntity);
	}

	const LambdaEngine::Entity KilledEntity;
};