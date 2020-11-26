#pragma once

#include "Application/API/Events/Event.h"

enum EServerState
{
	SERVER_STATE_LOBBY,
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

constexpr FORCEINLINE const char* ServerStateToString(EServerState state)
{
	switch (state)
	{
	case EServerState::SERVER_STATE_LOBBY:		return "Lobby";
	case EServerState::SERVER_STATE_LOADING:	return "Loading";
	case EServerState::SERVER_STATE_PLAYING:	return "Playing";
	default: return "NONE";
	}
}