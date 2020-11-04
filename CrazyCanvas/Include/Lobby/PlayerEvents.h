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

struct PlayerChatEvent : public LambdaEngine::Event
{
public:
	inline PlayerChatEvent(const Player& player, const LambdaEngine::String& message)
		: Event(),
		Player(player),
		Message(message)
	{
	}

	DECLARE_EVENT_TYPE(PlayerChatEvent);

	virtual LambdaEngine::String ToString() const override
	{
		return LambdaEngine::String("PlayerChatEvent=" + Player.GetName());
	}

public:
	const Player& Player;
	const LambdaEngine::String& Message;
};

struct PlayerChatRecapEvent : public LambdaEngine::Event
{
public:
	inline PlayerChatRecapEvent(const LambdaEngine::String& message)
		: Event(),
		Message(message)
	{
	}

	DECLARE_EVENT_TYPE(PlayerChatEvent);

	virtual LambdaEngine::String ToString() const override
	{
		return LambdaEngine::String("PlayerChatRecapEvent");
	}

public:
	const LambdaEngine::String& Message;
};

struct SystemChatEvent : public LambdaEngine::Event
{
public:
	inline SystemChatEvent(const LambdaEngine::String& message)
		: Event(),
		Message(message)
	{
	}

	DECLARE_EVENT_TYPE(SystemChatEvent);

	virtual LambdaEngine::String ToString() const override
	{
		return LambdaEngine::String("SystemChatEvent");
	}

public:
	const LambdaEngine::String& Message;
};