#include "States/MultiplayerState.h"

#include "States/LobbyState.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "GUI/GUIHelpers.h"

#include "Resources/ResourceCatalog.h"

#include "Application/API/Events/EventQueue.h"

#include "Application/API/CommonApplication.h"
#include "World/Player/PlayerActionSystem.h"
#include "Input/API/Input.h"

#include "Lobby/ServerManager.h"

#include "Multiplayer/ClientHelper.h"

using namespace LambdaEngine;

MultiplayerState::MultiplayerState() : 
	m_IsManualConnection(false)
{

}

MultiplayerState::~MultiplayerState()
{
	EventQueue::UnregisterEventHandler<ClientConnectedEvent>(this, &MultiplayerState::OnClientConnected);
	EventQueue::UnregisterEventHandler<ClientDisconnectedEvent>(this, &MultiplayerState::OnClientDisconnected);
	EventQueue::UnregisterEventHandler<ServerOnlineEvent>(this, &MultiplayerState::OnServerOnlineEvent);
	EventQueue::UnregisterEventHandler<ServerOfflineEvent>(this, &MultiplayerState::OnServerOfflineEvent);
	EventQueue::UnregisterEventHandler<ServerUpdatedEvent>(this, &MultiplayerState::OnServerUpdatedEvent);

	m_MultiplayerGUI.Reset();
	m_View.Reset();
}

void MultiplayerState::Init()
{
	EventQueue::RegisterEventHandler<ClientConnectedEvent>(this, &MultiplayerState::OnClientConnected);
	EventQueue::RegisterEventHandler<ClientDisconnectedEvent>(this, &MultiplayerState::OnClientDisconnected);
	EventQueue::RegisterEventHandler<ServerOnlineEvent>(this, &MultiplayerState::OnServerOnlineEvent);
	EventQueue::RegisterEventHandler<ServerOfflineEvent>(this, &MultiplayerState::OnServerOfflineEvent);
	EventQueue::RegisterEventHandler<ServerUpdatedEvent>(this, &MultiplayerState::OnServerUpdatedEvent);

	CommonApplication::Get()->SetMouseVisibility(true);
	PlayerActionSystem::SetMouseEnabled(false);
	Input::PushInputMode(EInputLayer::GUI);

	DisablePlaySessionsRenderstages();
	ResourceManager::GetMusic(ResourceCatalog::MAIN_MENU_MUSIC_GUID)->Play();

	m_MultiplayerGUI = *new MultiplayerGUI(this);
	m_View = Noesis::GUI::CreateView(m_MultiplayerGUI);
	LambdaEngine::GUIApplication::SetView(m_View);


	const THashTable<uint64, ServerInfo>& serversLAN = ServerManager::GetServersLAN();
	const THashTable<uint64, ServerInfo>& serversWAN = ServerManager::GetServersWAN();

	for (auto& pair : serversLAN)
		m_MultiplayerGUI->AddServerLAN(pair.second);

	for (auto& pair : serversWAN)
		m_MultiplayerGUI->AddServerSaved(pair.second);
}

void MultiplayerState::Tick(LambdaEngine::Timestamp delta)
{

}

bool MultiplayerState::OnClientConnected(const LambdaEngine::ClientConnectedEvent& event)
{
	if (m_IsManualConnection)
	{
		ServerInfo serverInfo;
		serverInfo.EndPoint = event.pClient->GetEndPoint();
		serverInfo.Name		= "";
		ServerManager::RegisterNewServer(serverInfo);
	}

	State* pLobbyState = DBG_NEW LobbyState(m_MultiplayerGUI->GetPlayerName(), m_MultiplayerGUI->HasHostedServer());
	StateManager::GetInstance()->EnqueueStateTransition(pLobbyState, STATE_TRANSITION::POP_AND_PUSH);

	return false;
}

bool MultiplayerState::OnClientDisconnected(const LambdaEngine::ClientDisconnectedEvent& event)
{
	return false;
}

bool MultiplayerState::OnServerOnlineEvent(const ServerOnlineEvent& event)
{
	const ServerInfo& serverInfo = event.Server;

	if (serverInfo.IsLAN)
		m_MultiplayerGUI->AddServerLAN(serverInfo);
	else
		m_MultiplayerGUI->AddServerSaved(serverInfo);

	return true;
}

bool MultiplayerState::OnServerOfflineEvent(const ServerOfflineEvent& event)
{
	const ServerInfo& serverInfo = event.Server;

	if (serverInfo.IsLAN)
		m_MultiplayerGUI->RemoveServerLAN(serverInfo);
	else
		m_MultiplayerGUI->UpdateServerSaved(serverInfo);
		
	return true;
}

bool MultiplayerState::OnServerUpdatedEvent(const ServerUpdatedEvent& event)
{
	const ServerInfo& serverInfo = event.Server;

	if (serverInfo.IsLAN)
	{
		m_MultiplayerGUI->UpdateServerLAN(serverInfo);
	}
	else
	{
		if (serverInfo.ServerUID != event.ServerOld.ServerUID)
		{
			m_MultiplayerGUI->RemoveServerSaved(event.ServerOld);
			m_MultiplayerGUI->AddServerSaved(serverInfo);
		}
		else
		{
			m_MultiplayerGUI->UpdateServerSaved(serverInfo);
		}
	}

	return true;
}

bool MultiplayerState::ConnectToServer(const IPEndPoint& endPoint, bool isManual)
{
	m_IsManualConnection = isManual;
	return ClientSystem::GetInstance().Connect(endPoint);
}