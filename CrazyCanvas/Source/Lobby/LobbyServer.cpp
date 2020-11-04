#include "Lobby/LobbyServer.h"
#include "Lobby/PlayerManager.h"

#include "Networking/API/BinaryDecoder.h"
#include "Networking/API/BinaryEncoder.h"
#include "Game/Multiplayer/Server/ServerSystem.h"

#include "Multiplayer/Packet/PacketType.h"

using namespace LambdaEngine;

LobbyServer::LobbyServer()
{
	
}

LobbyServer::~LobbyServer()
{
	
}

bool LobbyServer::OnPlayerJoinedEvent(const PlayerJoinedEvent& event)
{
	const Player& player = event.Player;

	return true;
}

bool LobbyServer::OnPlayerLeftEvent(const PlayerLeftEvent& event)
{
	const Player& player = event.Player;

	return true;
}