#pragma once

#include "Lobby/LobbyBase.h"

class LobbyServer : public LobbyBase
{
public:
	LobbyServer();
	~LobbyServer();

private:
	bool OnPlayerJoinedEvent(const PlayerJoinedEvent& event) override;
	bool OnPlayerLeftEvent(const PlayerLeftEvent& event) override;

private:

};