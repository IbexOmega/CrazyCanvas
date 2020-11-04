#include "Lobby/LobbyClient.h"
#include "Lobby/PlayerManager.h"

#include "Networking/API/BinaryDecoder.h"

#include "Multiplayer/Packet/PacketType.h"

#include "Game/Multiplayer/Client/ClientSystem.h"

using namespace LambdaEngine;

LobbyClient::LobbyClient()
{
	
}

LobbyClient::~LobbyClient()
{
	
}

bool LobbyClient::OnPlayerJoinedEvent(const PlayerJoinedEvent& event)
{
	const Player& player = event.Player;

	return true;
}

bool LobbyClient::OnPlayerLeftEvent(const PlayerLeftEvent& event)
{
	const Player& player = event.Player;

	return true;
}