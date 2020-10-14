#pragma once
#include "Game/State.h"
#include "Game/StateManager.h"

#include "Engine/EngineConfig.h"

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

//#include <string>

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
	//m_pRoot = Noesis::GUI::LoadXaml<Grid>(xamlFile.c_str());

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

bool LobbyGUI::ConnectEvent(Noesis::BaseComponent* source, const char* event, const char* handler)
{
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonBackClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonConnectClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonRefreshClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonErrorOKClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonErrorClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonHostGameClick);
	return false;
}

void LobbyGUI::OnButtonBackClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	State* pMainMenuState = DBG_NEW MainMenuState();
	StateManager::GetInstance()->EnqueueStateTransition(pMainMenuState, STATE_TRANSITION::POP_AND_PUSH);
}

bool LobbyGUI::OnLANServerFound(const LambdaEngine::ServerDiscoveredEvent& event)
{
	BinaryDecoder* pDecoder = event.pDecoder;
	const IPEndPoint* pEndPoint = event.pEndPoint;

	ServerInfo newInfo;
	newInfo.Ping = 0;
	newInfo.LastUpdate = EngineLoop::GetTimeSinceStart();

	pDecoder->ReadUInt8(newInfo.Players);
	pDecoder->ReadString(newInfo.Name);
	pDecoder->ReadString(newInfo.MapName);

	const auto& pair = m_Servers.find(event.ServerUID);

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


	//m_ServerList.AddServerItem()

	//m_ServerList.AddLocalServerItem(pServerGrid, serverName.c_str(), mapName.c_str(), std::to_string(players).c_str(), true);

	//LOG_INFO("Found server with name %s with %u players", serverName.c_str(), players);
	return false;
}

void LobbyGUI::OnButtonConnectClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	LOG_MESSAGE(FrameworkElement::FindName<TextBox>("IP_ADDRESS")->GetText());

	IPAddress* pIP = IPAddress::Get(FrameworkElement::FindName<TextBox>("IP_ADDRESS")->GetText());

	/*if (!ClientSystem::GetInstance().Connect(pIP))
	{
		LOG_MESSAGE("Client already in use");
		return;
	}*/
	// Start Connecting animation

	LambdaEngine::GUIApplication::SetView(nullptr);

	SetRenderStagesActive();

	State* pPlayState = DBG_NEW PlaySessionState(pIP);
	StateManager::GetInstance()->EnqueueStateTransition(pPlayState, STATE_TRANSITION::POP_AND_PUSH);
}

void LobbyGUI::OnButtonRefreshClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	Grid* pServerGrid = FrameworkElement::FindName<Grid>("FIND_SERVER_CONTAINER");

	TabItem* pLocalServers = FrameworkElement::FindName<TabItem>("LOCAL");

	//m_ServerList.AddSavedServerItem(pServerGrid,  "BajsKorv", "BajsApa", "69", true);
}

void LobbyGUI::OnButtonErrorClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	TabItem* pLocalServers = FrameworkElement::FindName<TabItem>("LOCAL");

	//m_ServerList.UpdateServerItems("Bajs", "Bajs", "67", false, pLocalServers->GetIsSelected());

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

		State* pPlaySessionState = DBG_NEW PlaySessionState(NetworkUtils::GetLocalAddress());
		StateManager::GetInstance()->EnqueueStateTransition(pPlaySessionState, STATE_TRANSITION::POP_AND_PUSH);
	}


}

void LobbyGUI::SetRenderStagesActive()
{
	RenderSystem::GetInstance().SetRenderStageSleeping("SKYBOX_PASS",				false);
	RenderSystem::GetInstance().SetRenderStageSleeping("DEFERRED_GEOMETRY_PASS",	false);
	RenderSystem::GetInstance().SetRenderStageSleeping("DIRL_SHADOWMAP",			false);
	RenderSystem::GetInstance().SetRenderStageSleeping("FXAA",						false);
	RenderSystem::GetInstance().SetRenderStageSleeping("POINTL_SHADOW",				false);
	RenderSystem::GetInstance().SetRenderStageSleeping("SKYBOX_PASS",				false);
	RenderSystem::GetInstance().SetRenderStageSleeping("SHADING_PASS",				false);
	RenderSystem::GetInstance().SetRenderStageSleeping("RENDER_STAGE_NOESIS_GUI",	true);

	/*if (m_RayTracingEnabled)
		RenderSystem::GetInstance().SetRenderStageSleeping("RAY_TRACING", m_RayTracingSleeping);*/

}

void LobbyGUI::ErrorPopUp(ErrorCode errorCode)
{
	TextBlock* pTextBox = FrameworkElement::FindName<TextBlock>("ERROR_BOX_TEXT");
	
	switch (errorCode)
	{
	case CONNECT_ERROR:		pTextBox->SetText("Couldn't Connect To server"); break;
	case HOST_ERROR:		pTextBox->SetText("Couldn't Host Server");		break;
	case OTHER_ERROR:		pTextBox->SetText("Something Went Wrong");		break;
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
