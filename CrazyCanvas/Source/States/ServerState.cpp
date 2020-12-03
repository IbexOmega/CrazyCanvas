#include "States/ServerState.h"

#include "Memory/API/Malloc.h"

#include "Log/Log.h"

#include "Input/API/Input.h"

#include "Application/API/PlatformMisc.h"
#include "Application/API/CommonApplication.h"
#include "Application/API/PlatformConsole.h"
#include "Application/API/Window.h"
#include "Application/API/Events/EventQueue.h"

#include "Threading/API/Thread.h"

#include "Networking/API/PlatformNetworkUtils.h"

#include "Math/Random.h"

#include <argh/argh.h>

#include "Teams/TeamHelper.h"

#include "Game/Multiplayer/Server/ServerSystem.h"

#include "ECS/ECSCore.h"

#include "World/LevelManager.h"

#include "Match/Match.h"

#include "Multiplayer/Packet/PacketType.h"

#include "ECS/Systems/Match/ServerFlagSystem.h"

#include "Multiplayer/ServerHelper.h"

#include "Lobby/PlayerManagerServer.h"

#include "Chat/ChatManager.h"

#include <windows.h>
#include <Lmcons.h>

using namespace LambdaEngine;

EServerState ServerState::s_State = SERVER_STATE_LOBBY;

ServerState::ServerState(const std::string& clientHostID) :
	m_MultiplayerServer(),
	m_ClientHostID(0),
	m_MapName()
{
	if (!clientHostID.empty())
		m_ClientHostID = std::stoi(clientHostID);
	else
		m_ClientHostID = Random::Int32(0);

	DWORD length = UNLEN + 1;
	char buffer[UNLEN + 1];
	GetUserNameA(buffer, &length);

	String name = "Rum 1";//buffer;
	//name += "'s server";
	strcpy(m_GameSettings.ServerName, name.c_str());
	m_MapName = LevelManager::GetLevelNames()[0];
}

ServerState::~ServerState()
{
	EventQueue::UnregisterEventHandler<ServerDiscoveryPreTransmitEvent>(this, &ServerState::OnServerDiscoveryPreTransmit);
	EventQueue::UnregisterEventHandler<KeyPressedEvent>(this, &ServerState::OnKeyPressed);
	EventQueue::UnregisterEventHandler<PacketReceivedEvent<PacketGameSettings>>(this, &ServerState::OnPacketGameSettingsReceived);
	EventQueue::UnregisterEventHandler<PlayerJoinedEvent>(this, &ServerState::OnPlayerJoinedEvent);
	EventQueue::UnregisterEventHandler<PlayerStateUpdatedEvent>(this, &ServerState::OnPlayerStateUpdatedEvent);
	EventQueue::UnregisterEventHandler<ServerStateEvent>(this, &ServerState::OnServerStateEvent);
	EventQueue::UnregisterEventHandler<PlayerLeftEvent>(this, &ServerState::OnPlayerLeftEvent);
	EventQueue::UnregisterEventHandler<GameOverEvent>(this, &ServerState::OnGameOverEvent);
}

void ServerState::Init()
{
	EventQueue::RegisterEventHandler<ServerDiscoveryPreTransmitEvent>(this, &ServerState::OnServerDiscoveryPreTransmit);
	EventQueue::RegisterEventHandler<KeyPressedEvent>(this, &ServerState::OnKeyPressed);
	EventQueue::RegisterEventHandler<PacketReceivedEvent<PacketGameSettings>>(this, &ServerState::OnPacketGameSettingsReceived);
	EventQueue::RegisterEventHandler<PlayerJoinedEvent>(this, &ServerState::OnPlayerJoinedEvent);
	EventQueue::RegisterEventHandler<PlayerStateUpdatedEvent>(this, &ServerState::OnPlayerStateUpdatedEvent);
	EventQueue::RegisterEventHandler<ServerStateEvent>(this, &ServerState::OnServerStateEvent);
	EventQueue::RegisterEventHandler<PlayerLeftEvent>(this, &ServerState::OnPlayerLeftEvent);
	EventQueue::RegisterEventHandler<GameOverEvent>(this, &ServerState::OnGameOverEvent);

	CommonApplication::Get()->GetMainWindow()->SetTitle("Server");
	PlatformConsole::SetTitle("Server Console");

	m_MultiplayerServer.InitInternal();

	ServerSystem::GetInstance().Start();
}

bool ServerState::OnKeyPressed(const KeyPressedEvent& event)
{
	UNREFERENCED_VARIABLE(event);
	return false;
}

bool ServerState::OnServerDiscoveryPreTransmit(const LambdaEngine::ServerDiscoveryPreTransmitEvent& event)
{
	BinaryEncoder* pEncoder = event.pEncoder;
	ServerBase* pServer = event.pServer;

	pEncoder->WriteUInt8(pServer->GetClientCount());
	pEncoder->WriteUInt8(m_GameSettings.Players);
	pEncoder->WriteString(m_GameSettings.ServerName);
	pEncoder->WriteString(m_MapName);
	pEncoder->WriteInt32(m_ClientHostID);

	return true;
}

bool ServerState::OnPlayerJoinedEvent(const PlayerJoinedEvent& event)
{
	ServerHelper::SendToPlayer(event.pPlayer, m_GameSettings);
	return false;
}

void ServerState::Tick(Timestamp delta)
{
	m_MultiplayerServer.TickMainThreadInternal(delta);
}

void ServerState::FixedTick(LambdaEngine::Timestamp delta)
{
	m_MultiplayerServer.FixedTickMainThreadInternal(delta);
}

EServerState ServerState::GetState()
{
	return s_State;
}

bool ServerState::OnPacketGameSettingsReceived(const PacketReceivedEvent<PacketGameSettings>& event)
{	
	const PacketGameSettings& packet = event.Packet;

	const Player* pPlayer = PlayerManagerServer::GetPlayer(event.pClient);

	if (pPlayer->IsHost())
	{
		m_GameSettings = packet;
		m_MapName = LevelManager::GetLevelNames()[packet.MapID];
		ServerHelper::SendBroadcast(packet, nullptr, event.pClient);
		ServerHelper::SetMaxClients(packet.Players);

		if (PlayerManagerServer::GetPlayerCount() > packet.Players)
		{
			PlayerManagerServer::KickPlayers(PlayerManagerServer::GetPlayerCount() - packet.Players);
		}

		LOG_INFO("\nConfiguring Server With The Following Settings:");
		LOG_INFO("Name:       %s", packet.ServerName);
		LOG_INFO("Players:    %hhu", packet.Players);
		LOG_INFO("Map:        %s", m_MapName.c_str());
		LOG_INFO("GameMode:   %s", GameModeToString(packet.GameMode));
		LOG_INFO("MaxTime:    %hu", packet.MaxTime);
		LOG_INFO("FlagsToWin: %hhu", packet.FlagsToWin);
		LOG_INFO("Visible:    %s", packet.Visible ? "True" : "False");
		LOG_INFO("ChangeTeam: %s\n", packet.ChangeTeam ? "True" : "False");
		LOG_INFO("Team 1 Color Index: %d\n", packet.TeamColor1 );
		LOG_INFO("Team 2 Color Index: %d\n", packet.TeamColor2 );
	}
	else
	{
		LOG_ERROR("Player [%s] tried to change server settings while not being the host", pPlayer->GetName().c_str());
	}

	return true;
}

bool ServerState::OnPlayerStateUpdatedEvent(const PlayerStateUpdatedEvent& event)
{
	using namespace LambdaEngine;

	const Player* pPlayer = event.pPlayer;
	EGameState gameState = pPlayer->GetState();

	if (gameState == GAME_STATE_SETUP)
	{
		if (GetState() == SERVER_STATE_LOBBY)
		{
			SetState(SERVER_STATE_SETUP);
		}
		else
		{
			LOG_ERROR("Player [%s] tried to setup a match but the server is not in the lobby state", pPlayer->GetName().c_str());
		}
	}
	else if (gameState == GAME_STATE_LOADING)
	{
		TryLoadMatch();
	}
	else if (gameState == GAME_STATE_LOADED)
	{
		if (GetState() == SERVER_STATE_LOADING)
		{
			const THashTable<uint64, Player>& players = PlayerManagerServer::GetPlayers();
			for (auto& pair : players)
			{
				if (pair.second.GetState() != gameState)
					return false;
			}

			SetState(SERVER_STATE_PLAYING);
		}
		else
		{
			LOG_ERROR("Player [%s] tried to start a match but the server is not in the loading state", pPlayer->GetName().c_str());
		}
	}

	return true;
}

void ServerState::TryLoadMatch()
{
	if (GetState() != SERVER_STATE_SETUP)
		return;

	const THashTable<uint64, Player>& players = PlayerManagerServer::GetPlayers();
	for (auto& pair : players)
	{
		if (pair.second.GetState() != GAME_STATE_LOADING)
			return;
	}

	SetState(SERVER_STATE_LOADING);

	//Reset stats
	for (auto& pair : players)
	{
		const Player* pP = &pair.second;
		PlayerManagerServer::SetPlayerStats(pP, pP->GetTeam(), 0, 0, 0, 0);
	}

	// Load Match
	{
		const LambdaEngine::TArray<LambdaEngine::SHA256Hash>& levelHashes = LevelManager::GetLevelHashes();

		MatchDescription matchDescription =
		{
			.LevelHash = levelHashes[m_GameSettings.MapID],
			.GameMode = m_GameSettings.GameMode,
			.MaxScore = m_GameSettings.FlagsToWin,
		};

		if (!Match::CreateMatch(&matchDescription))
			LOG_ERROR("Failed to create match");

		Match::BeginLoading();
	}
}

bool ServerState::OnServerStateEvent(const ServerStateEvent& event)
{
	EServerState state = event.State;
	if (state == SERVER_STATE_LOBBY)
	{
		ChatManager::Clear();
		Match::ResetMatch();
		PlayerManagerServer::Reset();

		const THashTable<uint64, Player>& players = PlayerManagerServer::GetPlayers();
		for (auto& pair : players)
		{
			const Player* pPlayer = &pair.second;
			PlayerManagerServer::SetPlayerReady(pPlayer, false);
			PlayerManagerServer::SetPlayerState(pPlayer, EGameState::GAME_STATE_LOBBY);
		}

		ServerHelper::SetIgnoreNewClients(false);
	}
	else if (state == SERVER_STATE_SETUP)
	{
		ServerHelper::SetIgnoreNewClients(true);
		ServerHelper::SetTimeout(Timestamp::Seconds(15));
	}
	else if (state == SERVER_STATE_PLAYING)
	{
		ServerHelper::ResetTimeout();
		Match::StartMatch();
	}
	return true;
}

bool ServerState::OnPlayerLeftEvent(const PlayerLeftEvent& event)
{
	UNREFERENCED_VARIABLE(event);
	if (PlayerManagerServer::GetPlayerCount() == 1)
	{
		SetState(SERVER_STATE_LOBBY);
	}
	else
	{
		TryLoadMatch();
	}
	return false;
}

bool ServerState::OnGameOverEvent(const GameOverEvent& event)
{
	UNREFERENCED_VARIABLE(event);
	SetState(SERVER_STATE_LOBBY);
	return false;
}

void ServerState::SetState(EServerState state)
{
	s_State = state;
	ServerStateEvent event(s_State);
	EventQueue::SendEventImmediate(event);
}