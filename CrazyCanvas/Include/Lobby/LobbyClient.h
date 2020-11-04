#pragma once

#include "Lobby/LobbyBase.h"

class LobbyClient : public LobbyBase
{
public:
	LobbyClient();
	~LobbyClient();

private:
	bool OnPlayerJoinedEvent(const PlayerJoinedEvent& event) override;
	bool OnPlayerLeftEvent(const PlayerLeftEvent& event) override;

private:

};