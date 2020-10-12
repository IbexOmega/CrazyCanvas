#pragma once
#include "Game/State.h"
#include "Game/StateManager.h"

#include "Engine/EngineConfig.h"

#include "Game/ECS/Systems/Networking/ClientSystem.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"

#include "GUI/LobbyGUI.h"
#include "GUI/Core/GUIApplication.h"

#include "NoesisPCH.h"

#include "States/MainMenuState.h"
#include "States/NetworkingState.h"

//#include <string>

using namespace LambdaEngine;
using namespace Noesis;

LobbyGUI::LobbyGUI(const LambdaEngine::String& xamlFile) :
	m_HostGameDesc(),
	m_ServerList(xamlFile)
{
	Noesis::GUI::LoadComponent(this, xamlFile.c_str());
	//m_pRoot = Noesis::GUI::LoadXaml<Grid>(xamlFile.c_str());

	const char* ip = "192.168.1.65";

	FrameworkElement::FindName<TextBox>("IP_ADDRESS")->SetText(ip);
	//m_RayTracingEnabled = EngineConfig::GetBoolProperty("RayTracingEnabled");

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
	LOG_MESSAGE(FrameworkElement::FindName<TextBox>("IP_ADDRESS")->GetText());

	IPAddress* ip = IPAddress::Get(FrameworkElement::FindName<TextBox>("IP_ADDRESS")->GetText());

	if (!ClientSystem::GetInstance().Connect(ip))
	{
		LOG_MESSAGE("Client already in use");
		return;
	}
	// Start Connecting animation

	LambdaEngine::GUIApplication::SetView(nullptr);

	SetRenderStagesActive();

	State* pNetworkingState = DBG_NEW NetworkingState();
	StateManager::GetInstance()->EnqueueStateTransition(pNetworkingState, STATE_TRANSITION::POP_AND_PUSH);
}

void LobbyGUI::OnButtonRefreshClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	Grid* pServerGrid = FrameworkElement::FindName<Grid>("FIND_SERVER_CONTAINER");

	
	/*for (int i = 0; i < 4; i++)
	{
		Ptr<ListBoxItem> listBlock	= *new ListBoxItem();

		std::string text	= "Server " + std::to_string(i);

		listBlock->SetContent(text.c_str());
		listBlock->SetHorizontalAlignment(HorizontalAlignment_Center);
		listBlock->SetVerticalAlignment(VerticalAlignment_Center);

		pServerGrid->SetColumn(listBlock, 2);
		pServerGrid->SetRow(listBlock, i + 3);
	}*/

	LOG_MESSAGE(m_ServerList.GetList()->GetName());
	m_ServerList.AddServerItem(pServerGrid, 3, "BajsKorv", "BajsApa", true);

	ItemCollection* pCollection = m_ServerList.GetList()->GetItems();

	LOG_MESSAGE(pCollection->GetItemAt(0)->ToString().Str());
}

void LobbyGUI::OnButtonErrorClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	ErrorPopUp(OTHER_ERROR);
}

void LobbyGUI::OnButtonErrorOKClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	ErrorPopUpClose();
}

void LobbyGUI::OnButtonHostGameClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	PopulateServerInfo();
	//start Server with populated struct
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
	TextBlock* textBox = FrameworkElement::FindName<TextBlock>("ERROR_BOX_TEXT");
	
	switch (errorCode)
	{
	case CONNECT_ERROR:		textBox->SetText("Couldn't Connect To server"); break;
	case HOST_ERROR:		textBox->SetText("Couldn't Host Server");		break;
	case OTHER_ERROR:		textBox->SetText("Something Went Wrong");		break;
	}

	FrameworkElement::FindName<Grid>("ERROR_BOX_CONTAINER")->SetVisibility(Visibility_Visible);
}

void LobbyGUI::ErrorPopUpClose()
{
	FrameworkElement::FindName<Grid>("ERROR_BOX_CONTAINER")->SetVisibility(Visibility_Hidden);
}

const HostGameDescription& LobbyGUI::PopulateServerInfo()
{
	ComboBox* pCBPlayerCount = FrameworkElement::FindName<ComboBox>("PLAYER_NUMBER");
	ComboBoxItem* item = (ComboBoxItem*)pCBPlayerCount->GetSelectedItem();
	int8 playersNumber = (int8)std::stoi(item->GetContent()->ToString().Str());

	m_HostGameDesc.PlayersNumber = playersNumber;

	//Only necessary if we allow none to be chosen
	/*switch (playersNumber)
	{
	case 4:		m_HostGameDesc.PlayersNumber	= playersNumber; break;
	case 6:		m_HostGameDesc.PlayersNumber	= playersNumber; break;
	case 8:		m_HostGameDesc.PlayersNumber	= playersNumber; break;
	case 10:	m_HostGameDesc.PlayersNumber	= playersNumber; break;
	default: LOG_ERROR("No Player Count Set"); break;
	}*/

	ComboBox* pCBPMapOption = FrameworkElement::FindName<ComboBox>("MAP_OPTION");
	item = (ComboBoxItem*)pCBPlayerCount->GetSelectedItem();
	const char*  map = item->GetContent()->ToString().Str();

	if(map == "Standard")
		m_HostGameDesc.MapNumber = 0;

	LOG_MESSAGE("Player count %d", playersNumber);

	return m_HostGameDesc;
}

