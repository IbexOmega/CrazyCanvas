#pragma once

#include "Application/API/Events/Event.h"

#include "Lobby/Player.h"

#include "Log/Log.h"

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

struct PlayerStateUpdatedEvent : public PlayerBaseEvent
{
public:
	inline PlayerStateUpdatedEvent(const Player* pPlayerConst)
		: PlayerBaseEvent(pPlayerConst)
	{
		LOG_WARNING("[%s] CLIENT_STATE(%s)", pPlayerConst->GetName().c_str(), Player::GameStateToString(pPlayerConst->GetState()));
	}

	DECLARE_PLAYER_EVENT_TYPE(PlayerStateUpdatedEvent);
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

struct PlayerPingUpdatedEvent : public PlayerBaseEvent
{
public:
	inline PlayerPingUpdatedEvent(const Player* pPlayerConst)
		: PlayerBaseEvent(pPlayerConst)
	{
	}

	DECLARE_PLAYER_EVENT_TYPE(PlayerPingUpdatedEvent);
};

struct PlayerHostUpdatedEvent : public PlayerBaseEvent
{
public:
	inline PlayerHostUpdatedEvent(const Player* pPlayerConst)
		: PlayerBaseEvent(pPlayerConst)
	{
	}

	DECLARE_PLAYER_EVENT_TYPE(PlayerHostUpdatedEvent);
};

struct PlayerAliveUpdatedEvent : public PlayerBaseEvent
{
public:
	inline PlayerAliveUpdatedEvent(const Player* pPlayerConst, const Player* pPlayerKillerConst)
		: PlayerBaseEvent(pPlayerConst),
		pPlayerKiller(pPlayerKillerConst)
	{
	}

	DECLARE_PLAYER_EVENT_TYPE(PlayerAliveUpdatedEvent);

	const Player* pPlayerKiller;
};

struct PlayerReadyUpdatedEvent : public PlayerBaseEvent
{
public:
	inline PlayerReadyUpdatedEvent(const Player* pPlayerConst)
		: PlayerBaseEvent(pPlayerConst)
	{
	}

	DECLARE_PLAYER_EVENT_TYPE(PlayerReadyUpdatedEvent);
};