#include "Game/State.h"

#include "Engine/EngineConfig.h"
#include "Engine/EngineLoop.h"

#include "Networking/API/NetworkUtils.h"


#include "Game/Multiplayer/Client/ClientSystem.h"
#include "Game/Multiplayer/Server/ServerSystem.h"

#include "Multiplayer/Packet/PacketType.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"

#include "States/PlaySessionState.h"

#include "GUI/LobbyGUI.h"
#include "GUI/Core/GUIApplication.h"

#include "NoesisPCH.h"

#include "States/MainMenuState.h"
#include "States/ServerState.h"

#include <string>
#include <sstream>
#include <wchar.h>

#include "Math\Random.h"

#include "Multiplayer/ServerHostHelper.h"
#include "Multiplayer/ClientHelper.h"

#include "Application/API/Events/EventQueue.h"

using namespace LambdaEngine;
using namespace Noesis;

LobbyGUI::LobbyGUI() : 
	m_HostGameDesc(),
	m_ServerList(),
	m_Servers()
{
	Noesis::GUI::LoadComponent(this, "Lobby.xaml");

	EventQueue::RegisterEventHandler<ServerDiscoveredEvent>(this, &LobbyGUI::OnServerResponse);
	EventQueue::RegisterEventHandler<ClientConnectedEvent>(this, &LobbyGUI::OnClientConnected);

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
		HandleServerInfo(serverInfo, -1);
		ClientHelper::AddNetworkDiscoveryTarget(serverInfo.EndPoint.GetAddress());
	}
}

LobbyGUI::~LobbyGUI()
{
	EventQueue::UnregisterEventHandler<ServerDiscoveredEvent>(this, &LobbyGUI::OnServerResponse);
	EventQueue::UnregisterEventHandler<ClientConnectedEvent>(this, &LobbyGUI::OnClientConnected);
}

bool LobbyGUI::ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler)
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

void LobbyGUI::OnButtonBackClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	State* pMainMenuState = DBG_NEW MainMenuState();
	StateManager::GetInstance()->EnqueueStateTransition(pMainMenuState, STATE_TRANSITION::POP_AND_PUSH);
}

bool LobbyGUI::OnServerResponse(const LambdaEngine::ServerDiscoveredEvent& event)
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

	HandleServerInfo(serverInfo, clientHostID);

	return true;
}

void LobbyGUI::HandleServerInfo(ServerInfo& serverInfo, int32 clientHostID, bool forceSave)
{
	ServerInfo& currentInfo = m_Servers[serverInfo.EndPoint.GetAddress()];

	if (ServerHostHelper::GetClientHostID() == clientHostID)
	{
		SetRenderStagesActive();

		State* pPlaySessionState = DBG_NEW PlaySessionState(false, serverInfo.EndPoint);
		StateManager::GetInstance()->EnqueueStateTransition(pPlaySessionState, STATE_TRANSITION::POP_AND_PUSH);
	}

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

bool LobbyGUI::OnClientConnected(const LambdaEngine::ClientConnectedEvent& event)
{
	if (m_HasHostedServer)
	{
		PacketConfigureServer packet;
		packet.AuthenticationID	= ServerHostHelper::GetAuthenticationHostID();
		packet.MapID			= m_HostGameDesc.MapNumber;
		packet.Players			= m_HostGameDesc.PlayersNumber;

		ClientHelper::Send(packet);
	}

	return false;
}

void LobbyGUI::FixedTick(LambdaEngine::Timestamp delta)
{
	UNREFERENCED_VARIABLE(delta);
	CheckServerStatus();
}

void LobbyGUI::OnButtonConnectClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
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

		HandleServerInfo(serverInfo, -1, true);

		LambdaEngine::GUIApplication::SetView(nullptr);

		SetRenderStagesActive();

		State* pPlayState = DBG_NEW PlaySessionState(false, endPoint);
		StateManager::GetInstance()->EnqueueStateTransition(pPlayState, STATE_TRANSITION::POP_AND_PUSH);
	}
	else
	{
		ErrorPopUp(CONNECT_ERROR_INVALID);
	}
}

void LobbyGUI::OnButtonRefreshClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	// Grid* pServerGrid = FrameworkElement::FindName<Grid>("FIND_SERVER_CONTAINER");

	// TabItem* pLocalServers = FrameworkElement::FindName<TabItem>("LOCAL");
}

void LobbyGUI::OnButtonErrorClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	ErrorPopUp(OTHER_ERROR);
}

void LobbyGUI::OnButtonErrorOKClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);


	ErrorPopUpClose();
}

void LobbyGUI::OnButtonHostGameClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	PopulateServerInfo();


	if (!CheckServerSettings(m_HostGameDesc))
	{
		ErrorPopUp(HOST_ERROR);
	}
	else if(!m_HasHostedServer)
	{
		//start Server with populated struct
		NotiPopUP(HOST_NOTIFICATION);

		m_HasHostedServer = true;

		StartUpServer("../Build/bin/Debug-windows-x86_64-x64/CrazyCanvas/Server.exe", "--state=server");
		//LambdaEngine::GUIApplication::SetView(nullptr);
	}
}

void LobbyGUI::OnButtonJoinClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
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

bool LobbyGUI::JoinSelectedServer(Noesis::Grid* pGrid)
{
	for (auto& server : m_Servers)
	{
		const ServerInfo& serverInfo = server.second;

		if (serverInfo.GridUI == pGrid)
		{
			if (serverInfo.IsOnline)
			{
				LambdaEngine::GUIApplication::SetView(nullptr);

				SetRenderStagesActive();

				State* pPlaySessionState = DBG_NEW PlaySessionState(false, serverInfo.EndPoint);
				StateManager::GetInstance()->EnqueueStateTransition(pPlaySessionState, STATE_TRANSITION::POP_AND_PUSH);
				return true;
			}
			return false;
		}
	}
	return false;
}

bool LobbyGUI::CheckServerStatus()
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

bool LobbyGUI::StartUpServer(const std::string& applicationName, const std::string& commandLine)
{
	//additional Info
	STARTUPINFOA lpStartupInfo;
	PROCESS_INFORMATION lpProcessInfo;
	
	uint32 randAuthenticationID = Random::UInt32(0, UINT32_MAX / 2);
	uint32 randClientHostID = Random::UInt32(0, UINT32_MAX / 2);

	ServerHostHelper::SetAuthenticationID(randAuthenticationID);
	ServerHostHelper::SetClientHostID(randClientHostID);

	std::string authenticationID = std::to_string(randAuthenticationID);
	std::string clientHostID = std::to_string(randClientHostID);

	std::string finalCLine = applicationName + " " + commandLine + " " + clientHostID + " " + authenticationID;

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

void LobbyGUI::SetRenderStagesActive()
{
	RenderSystem::GetInstance().SetRenderStageSleeping("SKYBOX_PASS",						false);
	RenderSystem::GetInstance().SetRenderStageSleeping("DEFERRED_GEOMETRY_PASS",			false);
	RenderSystem::GetInstance().SetRenderStageSleeping("DEFERRED_GEOMETRY_PASS_MESH_PAINT", false);
	RenderSystem::GetInstance().SetRenderStageSleeping("DIRL_SHADOWMAP",					false);
	RenderSystem::GetInstance().SetRenderStageSleeping("FXAA",								false);
	RenderSystem::GetInstance().SetRenderStageSleeping("POINTL_SHADOW",						false);
	RenderSystem::GetInstance().SetRenderStageSleeping("SKYBOX_PASS",						false);
	RenderSystem::GetInstance().SetRenderStageSleeping("PLAYER_PASS",						false);
	RenderSystem::GetInstance().SetRenderStageSleeping("SHADING_PASS",						false);
	RenderSystem::GetInstance().SetRenderStageSleeping("RENDER_STAGE_NOESIS_GUI",			false);
	RenderSystem::GetInstance().SetRenderStageSleeping("RAY_TRACING",						false);
}

void LobbyGUI::ErrorPopUp(PopUpCode errorCode)
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

void LobbyGUI::NotiPopUP(PopUpCode notificationCode)
{
	TextBlock* pTextBox = FrameworkElement::FindName<TextBlock>("NOTIFICATION_BOX_TEXT");

	switch (notificationCode)
	{
	case HOST_NOTIFICATION:	pTextBox->SetText("Server Starting...");	break;
	case JOIN_NOTIFICATION:	pTextBox->SetText("Joining Server...");		break;
	}

	FrameworkElement::FindName<Grid>("NOTIFICATION_BOX_CONTAINER")->SetVisibility(Visibility_Visible);
}

void LobbyGUI::ErrorPopUpClose()
{
	FrameworkElement::FindName<Grid>("ERROR_BOX_CONTAINER")->SetVisibility(Visibility_Hidden);
}

void LobbyGUI::NotiPopUpClose()
{
	FrameworkElement::FindName<Grid>("NOTIFICATION_BOX_CONTAINER")->SetVisibility(Visibility_Hidden);
}

bool LobbyGUI::CheckServerSettings(const HostGameDescription& serverSettings)
{
	if (serverSettings.PlayersNumber == -1)
		return false;
	else if (serverSettings.MapNumber == -1)
		return false;

	return true;
}

void LobbyGUI::PopulateServerInfo()
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