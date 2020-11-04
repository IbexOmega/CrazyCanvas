#pragma once

#include "Lobby/PlayerEvents.h"

#include "Application/API/Events/NetworkEvents.h"

class LobbyBase
{
public:
	LobbyBase();
	~LobbyBase();

private:
	virtual bool OnPlayerJoinedEvent(const PlayerJoinedEvent& event) = 0;
	virtual bool OnPlayerLeftEvent(const PlayerLeftEvent& event) = 0;
};