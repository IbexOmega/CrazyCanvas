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

#include <processthreadsapi.h>

STARTUPINFO lpStartupInfo;
PROCESS_INFORMATION lpProcessInfo;

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

	const char* pIP = "192.168.1.65";

	FrameworkElement::FindName<TextBox>("IP_ADDRESS")->SetText(pIP);
	//m_RayTracingEnabled = EngineConfig::GetBoolProperty("RayTracingEnabled");
	m_ServerList.Init(FrameworkElement::FindName<ListBox>("SAVED_SERVER_LIST"), FrameworkElement::FindName<ListBox>("LOCAL_SERVER_LIST"));
	ErrorPopUpClose();
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

	ServerInfo& currentInfo = m_Servers[event.ServerUID];

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

	State* pPlayState = DBG_NEW PlaySessionState(false, pIP);
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

	/*
	ZeroMemory(&lpStartupInfo, sizeof(lpStartupInfo));
	lpStartupInfo.cb = sizeof(lpStartupInfo);
	ZeroMemory(&lpProcessInfo, sizeof(lpProcessInfo));

	CreateProcess(L"Server.exe",
		L"--state server", NULL, NULL,
		NULL, NULL, NULL, NULL,
		&lpStartupInfo,
		&lpProcessInfo
	);*/

	// TabItem* pLocalServers = FrameworkElement::FindName<TabItem>("LOCAL");

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
		ErrorPopUp(HOST_ERROR);
	else
	{
		//start Server with populated struct
		ServerSystem::GetInstance().Start();

		LambdaEngine::GUIApplication::SetView(nullptr);

		SetRenderStagesActive();

		State* pPlaySessionState = DBG_NEW PlaySessionState(false, NetworkUtils::GetLocalAddress());
		StateManager::GetInstance()->EnqueueStateTransition(pPlaySessionState, STATE_TRANSITION::POP_AND_PUSH);
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
			StartSelectedServer(pSelectedItem);
		else
			ErrorPopUp(JOIN_ERROR);
	}
	else // From Saved Server List
	{
		ListBox* pBox = FrameworkElement::FindName<ListBox>("SAVED_SERVER_LIST");
		Grid* pSelectedItem = (Grid*)pBox->GetSelectedItem();

		if (pSelectedItem)
			StartSelectedServer(pSelectedItem);
		else
			ErrorPopUp(JOIN_ERROR);
	}
}

void LobbyGUI::StartSelectedServer(Noesis::Grid* pGrid)
{
	for (auto& server : m_Servers)
	{
		if (server.second.ServerGrid == pGrid)
		{
			LambdaEngine::GUIApplication::SetView(nullptr);

			SetRenderStagesActive();

			State* pPlaySessionState = DBG_NEW PlaySessionState(false, server.second.EndPoint.GetAddress());
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

void LobbyGUI::HostServer()
{
	/*
	NetworkSegment* pPacket = m_pClient->GetFreePacket(NetworkSegment::TYPE_ENTITY_CREATE);
	BinaryEncoder encoder3(pPacket);
	encoder3.WriteBool(true);
	encoder3.WriteInt32(0);
	encoder3.WriteVec3(glm::vec3(0, 2, 0));
	OnPacketReceived(m_pClient, pPacket);
	m_pClient->ReturnPacket(pPacket);*/
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
	RenderSystem::GetInstance().SetRenderStageSleeping("RENDER_STAGE_NOESIS_GUI",			true);
	RenderSystem::GetInstance().SetRenderStageSleeping("RAY_TRACING",						false);

}

void LobbyGUI::ErrorPopUp(ErrorCode errorCode)
{
	TextBlock* pTextBox = FrameworkElement::FindName<TextBlock>("ERROR_BOX_TEXT");

	switch (errorCode)
	{
	case CONNECT_ERROR:		pTextBox->SetText("Couldn't Connect To Server!");	break;
	case JOIN_ERROR:		pTextBox->SetText("No Server Selected!");			break;
	case HOST_ERROR:		pTextBox->SetText("Couldn't Host Server!");			break;
	case OTHER_ERROR:		pTextBox->SetText("Something Went Wrong!");			break;
	}

	FrameworkElement::FindName<Grid>("ERROR_BOX_CONTAINER")->SetVisibility(Visibility_Visible);
}

void LobbyGUI::ErrorPopUpClose()
{
	FrameworkElement::FindName<Grid>("ERROR_BOX_CONTAINER")->SetVisibility(Visibility_Hidden);
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
