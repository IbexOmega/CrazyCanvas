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

using namespace LambdaEngine;
using namespace Noesis;

LobbyGUI::LobbyGUI(const LambdaEngine::String& xamlFile)

{
	Noesis::GUI::LoadComponent(this, xamlFile.c_str());
	//m_pRoot = Noesis::GUI::LoadXaml<Grid>(xamlFile.c_str());

	const char* ip = "192.168.1.65";

	FrameworkElement::FindName<TextBox>("IP_ADDRESS")->SetText(ip);
	//m_RayTracingEnabled = EngineConfig::GetBoolProperty("RayTracingEnabled");
}

LobbyGUI::~LobbyGUI()
{

}

bool LobbyGUI::ConnectEvent(Noesis::BaseComponent* source, const char* event, const char* handler)
{
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonBackClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonConnectClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonRefreshClick);
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
	ErrorPopUp();
	for (int i = 0; i < 4; i++)
	{
		Ptr<TextBlock> textBlock	= *new TextBlock();
		Ptr<TextBlock> textBlock2	= *new TextBlock();

		char* text	= "Row Row Row Your Boat";
		char* text2 = "Gently Down The Stream";

		textBlock->SetText(text);
		textBlock->SetHorizontalAlignment(HorizontalAlignment_Center);
		textBlock->SetVerticalAlignment(VerticalAlignment_Center);
		
		textBlock2->SetText(text2);
		textBlock2->SetHorizontalAlignment(HorizontalAlignment_Center);
		textBlock2->SetVerticalAlignment(VerticalAlignment_Center);

		pServerGrid->GetChildren()->Add(textBlock);
		pServerGrid->GetChildren()->Add(textBlock2);

		pServerGrid->SetColumn(textBlock, 2);
		pServerGrid->SetColumn(textBlock2, 1);

		pServerGrid->SetRow(textBlock, i + 3);
		pServerGrid->SetRow(textBlock2, i + 3);
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

void LobbyGUI::ErrorPopUp()
{
	FrameworkElement::FindName<Grid>("ERROR_BOX")->SetVisibility(Visibility_Visible);
}

