#include "States/MultiplayerState.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"

<<<<<<<< HEAD:CrazyCanvas/Source/States/MultiplayerState.cpp
MultiplayerState::~MultiplayerState()
{
	m_MultiplayerGUI.Reset();
	m_View.Reset();
========
#include "Chat/ChatManager.h"

#include "Lobby/PlayerManagerClient.h"

#include "Application/API/Events/EventQueue.h"

#include "Multiplayer/ServerHostHelper.h"
#include "Multiplayer/ClientHelper.h"

using namespace LambdaEngine;

LobbyState::~LobbyState()
{
	EventQueue::UnregisterEventHandler<PlayerJoinedEvent>(this, &LobbyState::OnPlayerJoinedEvent);
	EventQueue::UnregisterEventHandler<PlayerLeftEvent>(this, &LobbyState::OnPlayerLeftEvent);
	EventQueue::UnregisterEventHandler<PlayerInfoUpdatedEvent>(this, &LobbyState::OnPlayerInfoUpdatedEvent);
>>>>>>>> origin/lobby-gui:CrazyCanvas/Source/States/LobbyState.cpp
}

void MultiplayerState::Init()
{
	EventQueue::RegisterEventHandler<PlayerJoinedEvent>(this, &LobbyState::OnPlayerJoinedEvent);
	EventQueue::RegisterEventHandler<PlayerLeftEvent>(this, &LobbyState::OnPlayerLeftEvent);
	EventQueue::RegisterEventHandler<PlayerInfoUpdatedEvent>(this, &LobbyState::OnPlayerInfoUpdatedEvent);

	RenderSystem::GetInstance().SetRenderStageSleeping("SKYBOX_PASS", true);
	RenderSystem::GetInstance().SetRenderStageSleeping("DEFERRED_GEOMETRY_PASS", true);
	RenderSystem::GetInstance().SetRenderStageSleeping("DEFERRED_GEOMETRY_PASS_MESH_PAINT", true);
	RenderSystem::GetInstance().SetRenderStageSleeping("DIRL_SHADOWMAP", true);
	RenderSystem::GetInstance().SetRenderStageSleeping("FXAA", true);
	RenderSystem::GetInstance().SetRenderStageSleeping("POINTL_SHADOW", true);
	RenderSystem::GetInstance().SetRenderStageSleeping("SKYBOX_PASS", true);
	RenderSystem::GetInstance().SetRenderStageSleeping("SHADING_PASS", true);

<<<<<<<< HEAD:CrazyCanvas/Source/States/MultiplayerState.cpp
	// m_MultiplayerGUI = *new MultiplayerGUI("Multiplayer.xaml");
	m_MultiplayerGUI = *new LobbyGUI();
	m_View = Noesis::GUI::CreateView(m_MultiplayerGUI);
	LambdaEngine::GUIApplication::SetView(m_View);
========
	/*m_LobbyGUI = *new LobbyGUI("Lobby.xaml");
	m_View = Noesis::GUI::CreateView(m_LobbyGUI);
	LambdaEngine::GUIApplication::SetView(m_View);*/

	ChatManager::SendChatMessage("This is a chat message");
	PlayerManagerClient::SetLocalPlayerReady(true);
>>>>>>>> origin/lobby-gui:CrazyCanvas/Source/States/LobbyState.cpp
}

void MultiplayerState::Tick(LambdaEngine::Timestamp delta)
{

}

void MultiplayerState::FixedTick(LambdaEngine::Timestamp delta)
{
<<<<<<<< HEAD:CrazyCanvas/Source/States/MultiplayerState.cpp
	// m_MultiplayerGUI->FixedTick(delta);
========

}

bool LobbyState::OnPlayerJoinedEvent(const PlayerJoinedEvent& event)
{
	return false;
}

bool LobbyState::OnPlayerLeftEvent(const PlayerLeftEvent& event)
{
	return false;
}

bool LobbyState::OnPlayerInfoUpdatedEvent(const PlayerInfoUpdatedEvent& event)
{
	const Player* pPlayer = event.pPlayer;

	LOG_MESSAGE("Player: %s is %sready", pPlayer->GetName().c_str(), pPlayer->GetState() == PLAYER_STATE_READY ? "" : "not ");
	return false;
>>>>>>>> origin/lobby-gui:CrazyCanvas/Source/States/LobbyState.cpp
}

void LobbyState::SendServerConfiguration()
{
	m_PacketGameSettings.AuthenticationID = ServerHostHelper::GetAuthenticationHostID();
	ClientHelper::Send(m_PacketGameSettings);
}