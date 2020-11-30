#include "States/LobbyState.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"

#include "Chat/ChatManager.h"

#include "Lobby/PlayerManagerClient.h"

#include "Application/API/Events/EventQueue.h"

#include "States/PlaySessionState.h"
#include "States/MainMenuState.h"

#include "GUI/GUIHelpers.h"

#include "Resources/ResourceCatalog.h"

#include "Application/API/CommonApplication.h"
#include "World/Player/PlayerActionSystem.h"
#include "Input/API/Input.h"

#include "Application/API/PlatformConsole.h"

using namespace LambdaEngine;

LobbyState::LobbyState(const PacketGameSettings& gameSettings, const Player* pPlayer) : 
	m_Name(pPlayer->GetName()),
	m_IsHost(pPlayer->IsHost()),
	m_IsReplayLobby(true),
	m_GameSettings(gameSettings)
{
	
}

LobbyState::LobbyState(const LambdaEngine::String& name, bool isHost) :
	m_Name(name),
	m_IsHost(isHost),
	m_IsReplayLobby(false),
	m_GameSettings()
{
	LambdaEngine::String defaultName = name.length() + 9 > (MAX_NAME_LENGTH - 1) ? name : (name + "'s server");
	strcpy(m_GameSettings.ServerName, defaultName.c_str());
}

LobbyState::~LobbyState()
{
	EventQueue::UnregisterEventHandler<PlayerJoinedEvent>(this, &LobbyState::OnPlayerJoinedEvent);
	EventQueue::UnregisterEventHandler<PlayerLeftEvent>(this, &LobbyState::OnPlayerLeftEvent);
	EventQueue::UnregisterEventHandler<PlayerStateUpdatedEvent>(this, &LobbyState::OnPlayerStateUpdatedEvent);
	EventQueue::UnregisterEventHandler<PlayerHostUpdatedEvent>(this, &LobbyState::OnPlayerHostUpdatedEvent);
	EventQueue::UnregisterEventHandler<PlayerPingUpdatedEvent>(this, &LobbyState::OnPlayerPingUpdatedEvent);
	EventQueue::UnregisterEventHandler<PlayerReadyUpdatedEvent>(this, &LobbyState::OnPlayerReadyUpdatedEvent);
	EventQueue::UnregisterEventHandler<PlayerScoreUpdatedEvent>(this, &LobbyState::OnPlayerScoreUpdatedEvent);
	EventQueue::UnregisterEventHandler<ChatEvent>(this, &LobbyState::OnChatEvent);
	EventQueue::UnregisterEventHandler<PacketReceivedEvent<PacketGameSettings>>(this, &LobbyState::OnPacketGameSettingsReceived);
	EventQueue::UnregisterEventHandler<ClientDisconnectedEvent>(this, &LobbyState::OnClientDisconnected);

	m_LobbyGUI.Reset();
	m_View.Reset();
}

void LobbyState::Init()
{
	EventQueue::RegisterEventHandler<PlayerJoinedEvent>(this, &LobbyState::OnPlayerJoinedEvent);
	EventQueue::RegisterEventHandler<PlayerLeftEvent>(this, &LobbyState::OnPlayerLeftEvent);
	EventQueue::RegisterEventHandler<PlayerStateUpdatedEvent>(this, &LobbyState::OnPlayerStateUpdatedEvent);
	EventQueue::RegisterEventHandler<PlayerHostUpdatedEvent>(this, &LobbyState::OnPlayerHostUpdatedEvent);
	EventQueue::RegisterEventHandler<PlayerPingUpdatedEvent>(this, &LobbyState::OnPlayerPingUpdatedEvent);
	EventQueue::RegisterEventHandler<PlayerReadyUpdatedEvent>(this, &LobbyState::OnPlayerReadyUpdatedEvent);
	EventQueue::RegisterEventHandler<PlayerScoreUpdatedEvent>(this, &LobbyState::OnPlayerScoreUpdatedEvent);
	EventQueue::RegisterEventHandler<ChatEvent>(this, &LobbyState::OnChatEvent);
	EventQueue::RegisterEventHandler<PacketReceivedEvent<PacketGameSettings>>(this, &LobbyState::OnPacketGameSettingsReceived);
	EventQueue::RegisterEventHandler<ClientDisconnectedEvent>(this, &LobbyState::OnClientDisconnected);
	
	CommonApplication::Get()->SetMouseVisibility(true);
	PlayerActionSystem::SetMouseEnabled(false);
	Input::PushInputMode(EInputLayer::GUI);

	DisablePlaySessionsRenderstages();
	ResourceManager::GetMusic(ResourceCatalog::MAIN_MENU_MUSIC_GUID)->Play();

	m_LobbyGUI = *new LobbyGUI(&m_GameSettings);
	m_View = Noesis::GUI::CreateView(m_LobbyGUI);
	LambdaEngine::GUIApplication::SetView(m_View);

	m_LobbyGUI->InitGUI();

	ChatManager::Clear();

	if (!m_IsReplayLobby)
	{
		PlatformConsole::SetTitle((String("Crazy Canvas Console - ") + m_Name).c_str());
		PlayerManagerClient::Reset();
		PlayerManagerClient::RegisterLocalPlayer(m_Name, m_IsHost);
	}
	else
	{
		const THashTable<uint64, Player>& players = PlayerManagerClient::GetPlayers();
		for (auto& pair : players)
		{
			const Player& player = pair.second;
			m_LobbyGUI->AddPlayer(player);
			if (player.IsHost())
			{
				m_LobbyGUI->UpdatePlayerHost(player);
			}
		}
	}
}

bool LobbyState::OnPlayerJoinedEvent(const PlayerJoinedEvent& event)
{
	m_LobbyGUI->AddPlayer(*event.pPlayer);
	return false;
}

bool LobbyState::OnPlayerLeftEvent(const PlayerLeftEvent& event)
{
	m_LobbyGUI->RemovePlayer(*event.pPlayer);
	return false;
}

bool LobbyState::OnPlayerStateUpdatedEvent(const PlayerStateUpdatedEvent& event)
{
	const Player* pPlayer = event.pPlayer;
	if (pPlayer->GetState() == GAME_STATE_SETUP)
	{
		if (pPlayer == PlayerManagerClient::GetPlayerLocal())
		{
			State* pStartingState = DBG_NEW PlaySessionState(m_GameSettings);
			StateManager::GetInstance()->EnqueueStateTransition(pStartingState, STATE_TRANSITION::POP_AND_PUSH);
		}
	}
	return false;
}

bool LobbyState::OnPlayerHostUpdatedEvent(const PlayerHostUpdatedEvent& event)
{
	m_LobbyGUI->UpdatePlayerHost(*event.pPlayer);
	return false;
}

bool LobbyState::OnPlayerPingUpdatedEvent(const PlayerPingUpdatedEvent& event)
{
	m_LobbyGUI->UpdatePlayerPing(*event.pPlayer);
	return false;
}

bool LobbyState::OnPlayerReadyUpdatedEvent(const PlayerReadyUpdatedEvent& event)
{
	m_LobbyGUI->UpdatePlayerReady(*event.pPlayer);
	return false;
}

bool LobbyState::OnPlayerScoreUpdatedEvent(const PlayerScoreUpdatedEvent& event)
{
	m_LobbyGUI->UpdatePlayerScore(*event.pPlayer);
	return false;
}

bool LobbyState::OnChatEvent(const ChatEvent& event)
{
	m_LobbyGUI->WriteChatMessage(event);
	return false;
}

bool LobbyState::OnPacketGameSettingsReceived(const PacketReceivedEvent<PacketGameSettings>& packet)
{
	m_LobbyGUI->UpdateSettings(packet.Packet);
	return false;
}

bool LobbyState::OnClientDisconnected(const ClientDisconnectedEvent& event)
{
	const String& reason = event.Reason;

	LOG_WARNING("PlaySessionState::OnClientDisconnected(Reason: %s)", reason.c_str());

	PlayerManagerClient::Reset();

	State* pMainMenuState = DBG_NEW MainMenuState();
	StateManager::GetInstance()->EnqueueStateTransition(pMainMenuState, STATE_TRANSITION::POP_AND_PUSH);

	return false;
}