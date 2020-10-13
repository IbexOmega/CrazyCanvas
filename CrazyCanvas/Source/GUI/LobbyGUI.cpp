#pragma once
#include "Game/State.h"
#include "Game/StateManager.h"

#include "Engine/EngineConfig.h"

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

using namespace LambdaEngine;
using namespace Noesis;

LobbyGUI::LobbyGUI(const LambdaEngine::String& xamlFile) :
	m_HostGameDesc(),
	m_ServerList(xamlFile)
{
	Noesis::GUI::LoadComponent(this, xamlFile.c_str());
	//m_pRoot = Noesis::GUI::LoadXaml<Grid>(xamlFile.c_str());

	const char* pIP = "192.168.1.65";

	FrameworkElement::FindName<TextBox>("IP_ADDRESS")->SetText(pIP);
	//m_RayTracingEnabled = EngineConfig::GetBoolProperty("RayTracingEnabled");
	m_ServerList.Init(FrameworkElement::FindName<ListBox>("SAVED_SERVER_LIST"));
	ErrorPopUpClose();
}

LobbyGUI::~LobbyGUI()
{

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

void LobbyGUI::OnButtonConnectClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	LOG_MESSAGE(FrameworkElement::FindName<TextBox>("IP_ADDRESS")->GetText());

	IPAddress* pIP = IPAddress::Get(FrameworkElement::FindName<TextBox>("IP_ADDRESS")->GetText());

	if (!ClientSystem::GetInstance().Connect(pIP))
	{
		LOG_MESSAGE("Client already in use");
		return;
	}
	// Start Connecting animation

	LambdaEngine::GUIApplication::SetView(nullptr);

	SetRenderStagesActive();

	State* pPlayState = DBG_NEW PlaySessionState(true);
	StateManager::GetInstance()->EnqueueStateTransition(pPlayState, STATE_TRANSITION::POP_AND_PUSH);
}

void LobbyGUI::OnButtonRefreshClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	Grid* pServerGrid = FrameworkElement::FindName<Grid>("FIND_SERVER_CONTAINER");

	m_ServerList.AddServerItem(pServerGrid,  "BajsKorv", "BajsApa", "69",true);
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
		ErrorPopUp(HOST_ERROR);
	else
	{
		//start Server with populated struct
		ServerSystem::GetInstance().Start();
	
		LambdaEngine::GUIApplication::SetView(nullptr);

		SetRenderStagesActive();

		State* pServerState = DBG_NEW ServerState();
		StateManager::GetInstance()->EnqueueStateTransition(pServerState, STATE_TRANSITION::POP_AND_PUSH);
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
