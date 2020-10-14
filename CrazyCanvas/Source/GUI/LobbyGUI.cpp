#pragma once
#include "Game/State.h"
#include "Game/StateManager.h"
#include "GUI/LobbyGUI.h"
#include "GUI/Core/GUIApplication.h"
#include "NoesisPCH.h"
#include "States/MainMenuState.h"

#include "Application/API/Events/EventQueue.h"

using namespace LambdaEngine;

LobbyGUI::LobbyGUI(const LambdaEngine::String& xamlFile)
{
	Noesis::GUI::LoadComponent(this, xamlFile.c_str());

	EventQueue::RegisterEventHandler<ServerDiscoveredEvent>(this, &LobbyGUI::OnServerFound);
}

LobbyGUI::~LobbyGUI()
{
	EventQueue::UnregisterEventHandler<ServerDiscoveredEvent>(this, &LobbyGUI::OnServerFound);
}

bool LobbyGUI::ConnectEvent(Noesis::BaseComponent* source, const char* event, const char* handler)
{
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonBackClick);
	return false;
}

void LobbyGUI::OnButtonBackClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	State* pMainMenuState = DBG_NEW MainMenuState();
	StateManager::GetInstance()->EnqueueStateTransition(pMainMenuState, STATE_TRANSITION::POP_AND_PUSH);
}

bool LobbyGUI::OnServerFound(const LambdaEngine::ServerDiscoveredEvent& event)
{
	BinaryDecoder* pDecoder = event.pDecoder;
	const IPEndPoint* pEndPoint = event.pEndPoint;

	uint8 players = 0;
	pDecoder->ReadUInt8(players);

	LOG_INFO("Found server at %s with %u players", pEndPoint->ToString().c_str(), players);
	return false;
}

