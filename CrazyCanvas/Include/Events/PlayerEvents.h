#pragma once

#include "Application/API/Events/Event.h"

#include "Lobby/Player.h"

#define DECLARE_PLAYER_EVENT_TYPE(Type) \
		DECLARE_EVENT_TYPE(Type); \
		public: \
			virtual LambdaEngine::String ToString() const override \
			{ \
				return LambdaEngine::String(#Type) + pPlayer->GetName(); \
			} \

struct PlayerBaseEvent : public LambdaEngine::Event
{
protected:
	inline PlayerBaseEvent(const Player* pPlayerConst)
		: Event(),
		pPlayer(pPlayerConst)
	{
	}

public:
	const Player* pPlayer;
};

struct PlayerJoinedEvent : public PlayerBaseEvent
{
public:
	inline PlayerJoinedEvent(const Player* pPlayerConst)
		: PlayerBaseEvent(pPlayerConst)
	{
	}

	DECLARE_PLAYER_EVENT_TYPE(PlayerJoinedEvent);
};

struct PlayerLeftEvent : public PlayerBaseEvent
{
public:
	inline PlayerLeftEvent(const Player* pPlayerConst)
		: PlayerBaseEvent(pPlayerConst)
	{
	}

	DECLARE_PLAYER_EVENT_TYPE(PlayerLeftEvent);
};

struct PlayerInfoUpdatedEvent : public PlayerBaseEvent
{
public:
	inline PlayerInfoUpdatedEvent(const Player* pPlayerConst)
		: PlayerBaseEvent(pPlayerConst)
	{
	}

	DECLARE_PLAYER_EVENT_TYPE(PlayerInfoUpdatedEvent);
};

struct PlayerStateUpdatedEvent : public PlayerBaseEvent
{
public:
	inline PlayerStateUpdatedEvent(const Player* pPlayerConst)
		: PlayerBaseEvent(pPlayerConst)
	{
	}

	DECLARE_PLAYER_EVENT_TYPE(PlayerStateUpdatedEvent);
};

struct PlayerTeamUpdatedEvent : public PlayerBaseEvent
{
public:
	inline PlayerTeamUpdatedEvent(const Player* pPlayerConst)
		: PlayerBaseEvent(pPlayerConst)
	{
	}

	DECLARE_PLAYER_EVENT_TYPE(PlayerTeamUpdatedEvent);
};

struct PlayerScoreUpdatedEvent : public PlayerBaseEvent
{
public:
	inline PlayerScoreUpdatedEvent(const Player* pPlayerConst)
		: PlayerBaseEvent(pPlayerConst)
	{
	}

	DECLARE_PLAYER_EVENT_TYPE(PlayerScoreUpdatedEvent);
};