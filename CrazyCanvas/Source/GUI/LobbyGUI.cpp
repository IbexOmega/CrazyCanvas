#include "Game/State.h"

#include "Engine/EngineConfig.h"
#include "Engine/EngineLoop.h"

#include "Networking/API/NetworkUtils.h"


#include "Game/Multiplayer/Client/ClientSystem.h"
#include "Game/Multiplayer/Server/ServerSystem.h"

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

#include "Application/API/Events/EventQueue.h"

using namespace LambdaEngine;
using namespace Noesis;

LobbyGUI::LobbyGUI(const LambdaEngine::String& xamlFile) :
	m_HostGameDesc(),
	m_ServerList(xamlFile),
	m_Servers()
{
	Noesis::GUI::LoadComponent(this, xamlFile.c_str());

	EventQueue::RegisterEventHandler<ServerDiscoveredEvent>(this, &LobbyGUI::OnLANServerFound);
	EventQueue::RegisterEventHandler<ClientConnectedEvent>(this, &LobbyGUI::OnClientConnected);

	const char* pIP = "192.168.1.65";

	FrameworkElement::FindName<TextBox>("IP_ADDRESS")->SetText(pIP);
	//m_RayTracingEnabled = EngineConfig::GetBoolProperty("RayTracingEnabled");
	m_ServerList.Init(FrameworkElement::FindName<ListBox>("SAVED_SERVER_LIST"), FrameworkElement::FindName<ListBox>("LOCAL_SERVER_LIST"));
	
	ErrorPopUpClose();
	NotiPopUpClose();
}

LobbyGUI::~LobbyGUI()
{
	EventQueue::UnregisterEventHandler<ServerDiscoveredEvent>(this, &LobbyGUI::OnLANServerFound);
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

bool LobbyGUI::OnLANServerFound(const LambdaEngine::ServerDiscoveredEvent& event)
{
	BinaryDecoder* pDecoder = event.pDecoder;

	ServerInfo newInfo;
	newInfo.Ping = 0;
	newInfo.LastUpdate = EngineLoop::GetTimeSinceStart();
	newInfo.EndPoint = *event.pEndPoint;
	newInfo.ServerUID = event.ServerUID;

	pDecoder->ReadUInt8(newInfo.Players);
	pDecoder->ReadString(newInfo.Name);
	pDecoder->ReadString(newInfo.MapName);
	int32 serverHostID = pDecoder->ReadInt32();

	ServerInfo& currentInfo = m_Servers[event.ServerUID];

	if (serverHostID == ClientSystem::GetInstance().GetServerHostID())
	{
		SetRenderStagesActive();

		State* pPlaySessionState = DBG_NEW PlaySessionState(NetworkUtils::GetLocalAddress());
		StateManager::GetInstance()->EnqueueStateTransition(pPlaySessionState, STATE_TRANSITION::POP_AND_PUSH);
	}

	if (currentInfo != newInfo)
	{
		currentInfo = newInfo;
		if (currentInfo.ServerGrid) // update current list
		{
			m_ServerList.UpdateServerItems(currentInfo);
		}
		else // add new item to list
		{
			Grid* pServerGrid = FrameworkElement::FindName<Grid>("FIND_SERVER_CONTAINER");

			currentInfo.ServerGrid = m_ServerList.AddLocalServerItem(pServerGrid, currentInfo, true);
		}
	}
	else
	{
		currentInfo = newInfo;
	}

	return true;
}

bool LobbyGUI::OnClientConnected(const LambdaEngine::ClientConnectedEvent& event)
{
	IClient* pClient = event.pClient;

	if (ClientSystem::GetInstance().GetClientHostID() != -1)
	{
		NetworkSegment* pPacket = pClient->GetFreePacket(NetworkSegment::TYPE_HOST_SERVER);
		BinaryEncoder encoder(pPacket);
		encoder.WriteInt8(m_HostGameDesc.PlayersNumber);
		encoder.WriteInt8(m_HostGameDesc.MapNumber);
		encoder.WriteInt32(ClientSystem::GetInstance().GetClientHostID());
		pClient->SendReliable(pPacket, nullptr);
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

	LOG_MESSAGE(FrameworkElement::FindName<TextBox>("IP_ADDRESS")->GetText());

	IPAddress* pIP = IPAddress::Get(FrameworkElement::FindName<TextBox>("IP_ADDRESS")->GetText());

	LambdaEngine::GUIApplication::SetView(nullptr);

	SetRenderStagesActive();

	State* pPlayState = DBG_NEW PlaySessionState(pIP);
	StateManager::GetInstance()->EnqueueStateTransition(pPlayState, STATE_TRANSITION::POP_AND_PUSH);
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

	SavedServerSystem::WriteIpsToFile("bajs");
	SavedServerSystem::WriteIpsToFile("HejSimon");

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
	if (pTab->GetIsSelected()) // From LAN Server List
	{
		ListBox* pBox = FrameworkElement::FindName<ListBox>("LOCAL_SERVER_LIST");
		Grid* pSelectedItem = (Grid*)pBox->GetSelectedItem();

		if (pSelectedItem)
		{
			NotiPopUP(JOIN_NOTIFICATION);
			JoinSelectedServer(pSelectedItem);
		}
		else
			ErrorPopUp(JOIN_ERROR);
	}
	else // From Saved Server List
	{
		ListBox* pBox = FrameworkElement::FindName<ListBox>("SAVED_SERVER_LIST");
		Grid* pSelectedItem = (Grid*)pBox->GetSelectedItem();

		if (pSelectedItem)
		{
			NotiPopUP(JOIN_NOTIFICATION);
			JoinSelectedServer(pSelectedItem);
		}
		else
			ErrorPopUp(JOIN_ERROR);
	}
}

void LobbyGUI::JoinSelectedServer(Noesis::Grid* pGrid)
{
	for (auto& server : m_Servers)
	{
		if (server.second.ServerGrid == pGrid)
		{
			LambdaEngine::GUIApplication::SetView(nullptr);

			SetRenderStagesActive();

			State* pPlaySessionState = DBG_NEW PlaySessionState(server.second.EndPoint.GetAddress());
			StateManager::GetInstance()->EnqueueStateTransition(pPlaySessionState, STATE_TRANSITION::POP_AND_PUSH);
		}
	}
}

bool LobbyGUI::CheckServerStatus()
{
	TArray<uint64> serversToRemove;

	Timestamp timeSinceStart = EngineLoop::GetTimeSinceStart();
	Timestamp deltaTime;

	for (auto& server : m_Servers)
	{
		deltaTime = timeSinceStart - server.second.LastUpdate;

		if (deltaTime.AsSeconds() > 5)
		{
			ListBox* pParentBox = (ListBox*)server.second.ServerGrid->GetParent();
			pParentBox->GetItems()->Remove(server.second.ServerGrid);

			serversToRemove.PushBack(server.second.ServerUID);
		}
	}

	for (uint64 id : serversToRemove)
	{
		m_Servers.erase(id);
	}

	return false;
}

bool LobbyGUI::StartUpServer(std::string pApplicationName, std::string pCommandLine)
{
	//additional Info
	STARTUPINFOA lpStartupInfo;
	PROCESS_INFORMATION lpProcessInfo;
	
	uint32 randClientSpecificID = Random::UInt32(0, UINT32_MAX / 2);
	uint32 randServerSpecificID = Random::UInt32(0, UINT32_MAX / 2);

	ClientSystem::GetInstance().SetServerHostID(randServerSpecificID);
	ClientSystem::GetInstance().SetClientHostID(randClientSpecificID);

	std::string hostServerSideID = std::to_string(randServerSpecificID);
	std::string hostClientSideID = std::to_string(randClientSpecificID);

	std::string finalCLine = pApplicationName + " " + pCommandLine + " " + hostServerSideID + " " + hostClientSideID;

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
	RenderSystem::GetInstance().SetRenderStageSleeping("SHADING_PASS",						false);
	RenderSystem::GetInstance().SetRenderStageSleeping("RENDER_STAGE_NOESIS_GUI",			false);
	RenderSystem::GetInstance().SetRenderStageSleeping("RAY_TRACING",						false);

}

void LobbyGUI::ErrorPopUp(PopUpCode errorCode)
{
	TextBlock* pTextBox = FrameworkElement::FindName<TextBlock>("ERROR_BOX_TEXT");

	switch (errorCode)
	{
	case CONNECT_ERROR:		pTextBox->SetText("Couldn't Connect To Server!");		break;
	case JOIN_ERROR:		pTextBox->SetText("No Server Selected!");				break;
	case HOST_ERROR:		pTextBox->SetText("Couldn't Host Server!");				break;
	case OTHER_ERROR:		pTextBox->SetText("Something Went Wrong!");				break;
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
