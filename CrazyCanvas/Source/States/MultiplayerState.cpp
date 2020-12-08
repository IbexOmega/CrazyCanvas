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

#include "Math/Random.h"

#include <Psapi.h>
#include "Utilities/StringUtilities.h"

using namespace LambdaEngine;

MultiplayerState::MultiplayerState() : 
	m_IsManualConnection(false),
	m_ClientHostID(-1)
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

	State* pLobbyState = DBG_NEW LobbyState(m_MultiplayerGUI->GetPlayerName(), HasHostedServer());
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
	{
		if (m_ClientHostID == serverInfo.ClientHostID)
		{
			ConnectToServer(serverInfo.EndPoint, false);
		}
		m_MultiplayerGUI->AddServerLAN(serverInfo);
	}	
	else
	{
		m_MultiplayerGUI->AddServerSaved(serverInfo);
	}
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

bool MultiplayerState::HasHostedServer() const
{
	return m_ClientHostID != -1;
}

void MultiplayerState::StartUpServer()
{
	if (HasHostedServer())
		return;

	static String commandLine = "--state=server";

	// Get application (.exe) path
	HANDLE processHandle = NULL;
	WString filePath;

	processHandle = GetCurrentProcess();
	if (processHandle != NULL)
	{
		TCHAR filename[MAX_PATH];
		if (GetModuleFileNameEx(processHandle, NULL, filename, MAX_PATH) > 0)
		{
			filePath = WString(filename);
		}
		else
		{
			LOG_ERROR("Failed to get current process file path - cannot start server");
			return;
		}
	}

	//additional Info
	STARTUPINFOA lpStartupInfo;
	PROCESS_INFORMATION lpProcessInfo;
	m_ClientHostID = Random::Int32();

	std::string finalCLine = ConvertToAscii(filePath) + " " + commandLine + " " + std::to_string(m_ClientHostID);

	// set the size of the structures
	ZeroMemory(&lpStartupInfo, sizeof(lpStartupInfo));
	lpStartupInfo.cb = sizeof(lpStartupInfo);
	ZeroMemory(&lpProcessInfo, sizeof(lpProcessInfo));

	SetLastError(0);

	if (!CreateProcessA(
		NULL,
		finalCLine.data(),	//Command line
		NULL,			// Process handle not inheritable
		NULL,			// Thread handle not inheritable
		NULL,
		NULL,			// No creation flags
		NULL,			// Use parent's environment block
		NULL,			// Use parent's starting directory
		&lpStartupInfo,
		&lpProcessInfo)
		)
	{
		int dError2 = GetLastError();

		LOG_ERROR("Create Process LastError: %d", dError2);
	}
	else
	{
		LOG_MESSAGE("Create Process Success");

		// Close process and thread handles. 
		CloseHandle(lpProcessInfo.hProcess);
		CloseHandle(lpProcessInfo.hThread);
	}
}