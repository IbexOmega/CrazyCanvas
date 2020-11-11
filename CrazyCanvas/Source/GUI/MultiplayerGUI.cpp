#include "Game/State.h"

#include "Engine/EngineConfig.h"
#include "Engine/EngineLoop.h"

#include "Networking/API/NetworkUtils.h"


#include "Game/Multiplayer/Client/ClientSystem.h"
#include "Game/Multiplayer/Server/ServerSystem.h"

#include "Multiplayer/Packet/PacketType.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"

#include "States/LobbyState.h"
#include "States/PlaySessionState.h"

#include "GUI/MultiplayerGUI.h"
#include "GUI/Core/GUIApplication.h"

#include "NoesisPCH.h"

#include "States/MainMenuState.h"
#include "States/ServerState.h"

#include <string>
#include <sstream>
#include <wchar.h>

#include "Math\Random.h"

#include "Multiplayer/ClientHelper.h"

#include "Application/API/Events/EventQueue.h"

#include "Lobby/PlayerManagerClient.h"

#include <windows.h>
#include <Lmcons.h>

using namespace LambdaEngine;
using namespace Noesis;

MultiplayerGUI::MultiplayerGUI(const LambdaEngine::String& xamlFile) :
	m_HostGameDesc(),
	m_ServerList(),
	m_Servers(),
	m_ClientHostID(-1)
{
	Noesis::GUI::LoadComponent(this, xamlFile.c_str());

	EventQueue::RegisterEventHandler<ServerDiscoveredEvent>(this, &MultiplayerGUI::OnServerResponse);
	EventQueue::RegisterEventHandler<ClientConnectedEvent>(this, &MultiplayerGUI::OnClientConnected);

	const char* pIP = "81.170.143.133:4444";

	FrameworkElement::FindName<TextBox>("IP_ADDRESS")->SetText(pIP);
	//m_RayTracingEnabled = EngineConfig::GetBoolProperty(EConfigOption::CONFIG_OPTION_RAY_TRACING);
	m_ServerList.Init(FrameworkElement::FindName<ListBox>("SAVED_SERVER_LIST"), FrameworkElement::FindName<ListBox>("LOCAL_SERVER_LIST"));
	
	ErrorPopUpClose();
	NotiPopUpClose();

	TArray<ServerInfo> serverInfos;
	uint16 defaultPort = (uint16)EngineConfig::GetUint32Property(EConfigOption::CONFIG_OPTION_NETWORK_PORT);
	SavedServerSystem::LoadServers(serverInfos, defaultPort);

	for (ServerInfo& serverInfo : serverInfos)
	{
		HandleServerInfo(serverInfo);
		ClientHelper::AddNetworkDiscoveryTarget(serverInfo.EndPoint.GetAddress());
	}

	// Use Host name as default In Game name
	DWORD length = UNLEN + 1;
	char name[UNLEN + 1];
	GetUserNameA(name, &length);
	FrameworkElement::FindName<TextBox>("IN_GAME_NAME")->SetText(name);

}

MultiplayerGUI::~MultiplayerGUI()
{
	EventQueue::UnregisterEventHandler<ServerDiscoveredEvent>(this, &MultiplayerGUI::OnServerResponse);
	EventQueue::UnregisterEventHandler<ClientConnectedEvent>(this, &MultiplayerGUI::OnClientConnected);
}

bool MultiplayerGUI::ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler)
{
	NS_CONNECT_EVENT_DEF(pSource, pEvent, pHandler);

	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonBackClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonConnectClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonRefreshClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonErrorOKClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonErrorClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonHostGameClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonJoinClick);
	return false;
}

void MultiplayerGUI::OnButtonBackClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	State* pMainMenuState = DBG_NEW MainMenuState();
	StateManager::GetInstance()->EnqueueStateTransition(pMainMenuState, STATE_TRANSITION::POP_AND_PUSH);
}

bool MultiplayerGUI::OnServerResponse(const LambdaEngine::ServerDiscoveredEvent& event)
{
	BinaryDecoder* pDecoder = event.pDecoder;

	ServerInfo serverInfo;
	serverInfo.Ping			= (uint16)event.Ping.AsMilliSeconds();
	serverInfo.LastUpdate	= EngineLoop::GetTimeSinceStart();
	serverInfo.EndPoint		= *event.pEndPoint;
	serverInfo.ServerUID	= event.ServerUID;
	serverInfo.IsLAN		= event.IsLAN;
	serverInfo.IsOnline		= true;

	pDecoder->ReadUInt8(serverInfo.Players);
	pDecoder->ReadString(serverInfo.Name);
	pDecoder->ReadString(serverInfo.MapName);
	int32 clientHostID = pDecoder->ReadInt32();

	HandleServerInfo(serverInfo);

	if (m_ClientHostID == clientHostID)
	{
		ClientSystem::GetInstance().Connect(serverInfo.EndPoint);
	}

	return true;
}

void MultiplayerGUI::HandleServerInfo(ServerInfo& serverInfo, bool forceSave)
{
	ServerInfo& currentInfo = m_Servers[serverInfo.EndPoint.GetAddress()];

	LambdaEngine::String oldName = currentInfo.Name;

	if (currentInfo != serverInfo)
	{
		currentInfo = serverInfo;
		if (currentInfo.GridUI)
		{
			m_ServerList.UpdateServerInfo(currentInfo);
		}
		else
		{
			Grid* pServerGrid = FrameworkElement::FindName<Grid>("FIND_SERVER_CONTAINER");
			m_ServerList.AddServer(pServerGrid, currentInfo);
		}
	}
	else
	{
		currentInfo = serverInfo;
	}

	if (oldName != serverInfo.Name || forceSave)
		SavedServerSystem::SaveServers(m_Servers);
}

bool MultiplayerGUI::HasHostedServer() const
{
	return m_ClientHostID != -1;
}

bool MultiplayerGUI::OnClientConnected(const LambdaEngine::ClientConnectedEvent& event)
{
	LambdaEngine::String inGameName = FrameworkElement::FindName<TextBox>("IN_GAME_NAME")->GetText();

	State* pLobbyState = DBG_NEW LobbyState(inGameName, HasHostedServer());
	StateManager::GetInstance()->EnqueueStateTransition(pLobbyState, STATE_TRANSITION::POP_AND_PUSH);

	return false;
}

void MultiplayerGUI::FixedTick(LambdaEngine::Timestamp delta)
{
	UNREFERENCED_VARIABLE(delta);
	CheckServerStatus();
}

void MultiplayerGUI::OnButtonConnectClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	LambdaEngine::String address	= FrameworkElement::FindName<TextBox>("IP_ADDRESS")->GetText();
	bool isEndpointValid			= false;
	uint16 defaultPort				= (uint16)EngineConfig::GetUint32Property(EConfigOption::CONFIG_OPTION_NETWORK_PORT);
	IPEndPoint endPoint				= IPEndPoint::Parse(address, defaultPort, &isEndpointValid);

	if (isEndpointValid)
	{
		IPAddress* pAddress = endPoint.GetAddress();
		ClientHelper::AddNetworkDiscoveryTarget(pAddress);

		ServerInfo& serverInfo = m_Servers[pAddress];
		serverInfo.EndPoint = endPoint;

		HandleServerInfo(serverInfo, true);
		ClientSystem::GetInstance().Connect(endPoint);
	}
	else
	{
		ErrorPopUp(CONNECT_ERROR_INVALID);
	}
}

void MultiplayerGUI::OnButtonRefreshClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	// Grid* pServerGrid = FrameworkElement::FindName<Grid>("FIND_SERVER_CONTAINER");

	// TabItem* pLocalServers = FrameworkElement::FindName<TabItem>("LOCAL");
}

void MultiplayerGUI::OnButtonErrorClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	ErrorPopUp(OTHER_ERROR);
}

void MultiplayerGUI::OnButtonErrorOKClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	ErrorPopUpClose();
}

void MultiplayerGUI::OnButtonHostGameClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	/*PopulateServerInfo();


	if (!CheckServerSettings(m_HostGameDesc))
	{
		ErrorPopUp(HOST_ERROR);
	}
	else*/ if(!HasHostedServer())
	{
		//start Server with populated struct
		NotiPopUP(HOST_NOTIFICATION);

#if defined(LAMBDA_CONFIG_DEBUG)
		StartUpServer("../Build/bin/Debug-windows-x86_64-x64/CrazyCanvas/Server.exe", "--state=server");
#elif defined(LAMBDA_CONFIG_RELEASE)
		StartUpServer("../Build/bin/Release-windows-x86_64-x64/CrazyCanvas/Server.exe", "--state=server");
#elif defined(LAMBDA_CONFIG_PRODUCTION)
		StartUpServer("../Build/bin/Production-windows-x86_64-x64/CrazyCanvas/Server.exe", "--state=server");
#endif
		//LambdaEngine::GUIApplication::SetView(nullptr);
	}
}

void MultiplayerGUI::OnButtonJoinClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	TabItem* pTab = FrameworkElement::FindName<TabItem>("LOCAL");
	const char* GUI_ID = pTab->GetIsSelected() ? "LOCAL_SERVER_LIST" : "SAVED_SERVER_LIST";
	ListBox* pBox = FrameworkElement::FindName<ListBox>(GUI_ID);
	Grid* pSelectedItem = (Grid*)pBox->GetSelectedItem();

	if (pSelectedItem)
	{
		if (JoinSelectedServer(pSelectedItem))
			NotiPopUP(JOIN_NOTIFICATION);
		else
			ErrorPopUp(JOIN_ERROR_OFFLINE);
	}
	else
	{
		ErrorPopUp(JOIN_ERROR);
	}
}

bool MultiplayerGUI::JoinSelectedServer(Noesis::Grid* pGrid)
{
	for (auto& server : m_Servers)
	{
		const ServerInfo& serverInfo = server.second;

		if (serverInfo.GridUI == pGrid)
		{
			if (serverInfo.IsOnline)
			{
				return ClientSystem::GetInstance().Connect(serverInfo.EndPoint);;
			}
			return false;
		}
	}
	return false;
}

bool MultiplayerGUI::CheckServerStatus()
{
	TArray<IPAddress*> serversToRemove;

	Timestamp timeSinceStart = EngineLoop::GetTimeSinceStart();
	Timestamp deltaTime;
	static Timestamp timeout = Timestamp::Seconds(5);

	for (auto& server : m_Servers)
	{
		deltaTime = timeSinceStart - server.second.LastUpdate;

		if (deltaTime > timeout)
		{
			ServerInfo& serverInfo = server.second;

			if (serverInfo.IsLAN)
			{
				ListBox* pParentBox = (ListBox*)serverInfo.GridUI->GetParent();
				pParentBox->GetItems()->Remove(serverInfo.GridUI);
				serversToRemove.PushBack(serverInfo.EndPoint.GetAddress());
			}
			else if(serverInfo.IsOnline)
			{
				serverInfo.IsOnline = false;
				m_ServerList.UpdateServerInfo(serverInfo);
			}
		}
	}

	for (IPAddress* address : serversToRemove)
		m_Servers.erase(address);

	return false;
}

bool MultiplayerGUI::StartUpServer(const std::string& applicationName, const std::string& commandLine)
{
	//additional Info
	STARTUPINFOA lpStartupInfo;
	PROCESS_INFORMATION lpProcessInfo;

	m_ClientHostID = Random::Int32();

	std::string finalCLine = applicationName + " " + commandLine + " " + std::to_string(m_ClientHostID);

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
		return false;
	}
	else
	{
		LOG_MESSAGE("Create Process Success");

		// Close process and thread handles. 
		CloseHandle(lpProcessInfo.hProcess);
		CloseHandle(lpProcessInfo.hThread);

		return true;
	}
}

void MultiplayerGUI::ErrorPopUp(PopUpCode errorCode)
{
	TextBlock* pTextBox = FrameworkElement::FindName<TextBlock>("ERROR_BOX_TEXT");

	switch (errorCode)
	{
	case CONNECT_ERROR:				pTextBox->SetText("Couldn't Connect To Server!");		break;
	case CONNECT_ERROR_INVALID:		pTextBox->SetText("The address format is invalid!");	break;
	case JOIN_ERROR:				pTextBox->SetText("No Server Selected!");				break;
	case JOIN_ERROR_OFFLINE:		pTextBox->SetText("Server is offline!");				break;
	case HOST_ERROR:				pTextBox->SetText("Couldn't Host Server!");				break;
	case OTHER_ERROR:				pTextBox->SetText("Something Went Wrong!");				break;
	}

	FrameworkElement::FindName<Grid>("ERROR_BOX_CONTAINER")->SetVisibility(Visibility_Visible);
}

void MultiplayerGUI::NotiPopUP(PopUpCode notificationCode)
{
	TextBlock* pTextBox = FrameworkElement::FindName<TextBlock>("NOTIFICATION_BOX_TEXT");

	switch (notificationCode)
	{
	case HOST_NOTIFICATION:	pTextBox->SetText("Server Starting...");	break;
	case JOIN_NOTIFICATION:	pTextBox->SetText("Joining Server...");		break;
	}

	FrameworkElement::FindName<Grid>("NOTIFICATION_BOX_CONTAINER")->SetVisibility(Visibility_Visible);
}

void MultiplayerGUI::ErrorPopUpClose()
{
	FrameworkElement::FindName<Grid>("ERROR_BOX_CONTAINER")->SetVisibility(Visibility_Hidden);
}

void MultiplayerGUI::NotiPopUpClose()
{
	FrameworkElement::FindName<Grid>("NOTIFICATION_BOX_CONTAINER")->SetVisibility(Visibility_Hidden);
}

bool MultiplayerGUI::CheckServerSettings(const HostGameDescription& serverSettings)
{
	if (serverSettings.PlayersNumber == -1)
		return false;
	else if (serverSettings.MapNumber == -1)
		return false;

	return true;
}

void MultiplayerGUI::PopulateServerInfo()
{
	ComboBox* pCBPlayerCount = FrameworkElement::FindName<ComboBox>("PLAYER_NUMBER");
	ComboBoxItem* pItem = (ComboBoxItem*)pCBPlayerCount->GetSelectedItem();
	int8 playersNumber = (int8)std::stoi(pItem->GetContent()->ToString().Str());

	ComboBox* pCBPMapOption = FrameworkElement::FindName<ComboBox>("MAP_OPTION");
	pItem = (ComboBoxItem*)pCBPMapOption->GetSelectedItem();
	const char*  pMap = pItem->GetContent()->ToString().Str();

	m_HostGameDesc.PlayersNumber = playersNumber;

	if(std::strcmp(pMap, "Standard") == 0)
		m_HostGameDesc.MapNumber = 0;

	LOG_MESSAGE("Player count %d", playersNumber);
	LOG_MESSAGE(pMap);
}