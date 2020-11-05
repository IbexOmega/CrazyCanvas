#pragma once

#include "Application/API/Events/Event.h"

#include "Lobby/Player.h"

struct PlayerJoinedEvent : public LambdaEngine::Event
{
public:
	inline PlayerJoinedEvent(const Player& player)
		: Event(),
		Player(player)
	{
	}

	DECLARE_EVENT_TYPE(PlayerJoinedEvent);

	virtual LambdaEngine::String ToString() const override
	{
		return LambdaEngine::String("PlayerJoinedEvent=" + Player.GetName());
	}

public:
	const Player& Player;
};

struct PlayerLeftEvent : public LambdaEngine::Event
{
public:
	inline PlayerLeftEvent(const Player& player)
		: Event(),
		Player(player)
	{
	}

	DECLARE_EVENT_TYPE(PlayerLeftEvent);

	virtual LambdaEngine::String ToString() const override
	{
		return LambdaEngine::String("PlayerLeftEvent=" + Player.GetName());
	}

public:
	const Player& Player;
};

struct PlayerInfoUpdatedEvent : public LambdaEngine::Event
{
public:
	inline PlayerInfoUpdatedEvent(const Player& player)
		: Event(),
		Player(player)
	{
	}

	DECLARE_EVENT_TYPE(PlayerLeftEvent);

	virtual LambdaEngine::String ToString() const override
	{
		return LambdaEngine::String("PlayerLeftEvent=" + Player.GetName());
	}

public:
	const Player& Player;
};