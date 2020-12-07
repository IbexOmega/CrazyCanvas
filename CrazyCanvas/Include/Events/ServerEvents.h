#pragma once

#include "Application/API/Events/Event.h"

#include "Lobby/ServerInfo.h"

enum EServerState
{
	SERVER_STATE_LOBBY,
	SERVER_STATE_SETUP,
	SERVER_STATE_LOADING,
	SERVER_STATE_PLAYING
};

struct ServerStateEvent : public LambdaEngine::Event
{
public:
	inline ServerStateEvent(EServerState state)
		: Event()
		, State(state)
	{
	}

	DECLARE_EVENT_TYPE(ServerStateEvent);

	virtual LambdaEngine::String ToString() const override
	{
		return LambdaEngine::String("ServerStateEvent");
	}

	EServerState State;
};

struct ServerOnlineEvent : public LambdaEngine::Event
{
public:
	inline ServerOnlineEvent(const ServerInfo& serverInfo)
		: Event()
		, Server(serverInfo)
	{
	}

	DECLARE_EVENT_TYPE(ServerOnlineEvent);

	virtual LambdaEngine::String ToString() const override
	{
		return LambdaEngine::String("ServerOnlineEvent");
	}

	const ServerInfo& Server;
};

struct ServerOfflineEvent : public LambdaEngine::Event
{
public:
	inline ServerOfflineEvent(const ServerInfo& serverInfo)
		: Event()
		, Server(serverInfo)
	{
	}

	DECLARE_EVENT_TYPE(ServerOfflineEvent);

	virtual LambdaEngine::String ToString() const override
	{
		return LambdaEngine::String("ServerOfflineEvent");
	}

	const ServerInfo& Server;
};

struct ServerUpdatedEvent : public LambdaEngine::Event
{
public:
	inline ServerUpdatedEvent(const ServerInfo& serverInfo, const ServerInfo& serverInfoOld)
		: Event()
		, Server(serverInfo)
		, ServerOld(serverInfoOld)
	{
	}

	DECLARE_EVENT_TYPE(ServerUpdatedEvent);

	virtual LambdaEngine::String ToString() const override
	{
		return LambdaEngine::String("ServerUpdatedEvent");
	}

	const ServerInfo& Server;
	const ServerInfo& ServerOld;
};

constexpr FORCEINLINE const char* ServerStateToString(EServerState state)
{
	switch (state)
	{
	case EServerState::SERVER_STATE_LOBBY:		return "Lobby";
	case EServerState::SERVER_STATE_SETUP:		return "Setup";
	case EServerState::SERVER_STATE_LOADING:	return "Loading";
	case EServerState::SERVER_STATE_PLAYING:	return "Playing";
	default: return "NONE";
	}
}