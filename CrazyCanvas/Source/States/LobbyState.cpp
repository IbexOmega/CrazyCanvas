#include "States/LobbyState.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"

#include "Chat/ChatManager.h"

#include "Lobby/PlayerManagerClient.h"

#include "Application/API/Events/EventQueue.h"

using namespace LambdaEngine;

LobbyState::LobbyState(const LambdaEngine::String& name) :
	m_Name(name)
{

}

LobbyState::~LobbyState()
{
	EventQueue::UnregisterEventHandler<PlayerJoinedEvent>(this, &LobbyState::OnPlayerJoinedEvent);
	EventQueue::UnregisterEventHandler<PlayerLeftEvent>(this, &LobbyState::OnPlayerLeftEvent);
	EventQueue::UnregisterEventHandler<PlayerStateUpdatedEvent>(this, &LobbyState::OnPlayerStateUpdatedEvent);
	EventQueue::UnregisterEventHandler<PlayerPingUpdatedEvent>(this, &LobbyState::OnPlayerPingUpdatedEvent);
	EventQueue::UnregisterEventHandler<ChatEvent>(this, &LobbyState::OnChatEvent);

	m_LobbyGUI.Reset();
	m_View.Reset();
}

void LobbyState::Init()
{
	EventQueue::RegisterEventHandler<PlayerJoinedEvent>(this, &LobbyState::OnPlayerJoinedEvent);
	EventQueue::RegisterEventHandler<PlayerLeftEvent>(this, &LobbyState::OnPlayerLeftEvent);
	EventQueue::RegisterEventHandler<PlayerStateUpdatedEvent>(this, &LobbyState::OnPlayerStateUpdatedEvent);
	EventQueue::RegisterEventHandler<PlayerPingUpdatedEvent>(this, &LobbyState::OnPlayerPingUpdatedEvent);
	EventQueue::RegisterEventHandler<ChatEvent>(this, &LobbyState::OnChatEvent);

	RenderSystem::GetInstance().SetRenderStageSleeping("SKYBOX_PASS", true);
	RenderSystem::GetInstance().SetRenderStageSleeping("DEFERRED_GEOMETRY_PASS", true);
	RenderSystem::GetInstance().SetRenderStageSleeping("DEFERRED_GEOMETRY_PASS_MESH_PAINT", true);
	RenderSystem::GetInstance().SetRenderStageSleeping("DIRL_SHADOWMAP", true);
	RenderSystem::GetInstance().SetRenderStageSleeping("FXAA", true);
	RenderSystem::GetInstance().SetRenderStageSleeping("POINTL_SHADOW", true);
	RenderSystem::GetInstance().SetRenderStageSleeping("SKYBOX_PASS", true);
	RenderSystem::GetInstance().SetRenderStageSleeping("SHADING_PASS", true);

	m_LobbyGUI = *new LobbyGUI();
	m_View = Noesis::GUI::CreateView(m_LobbyGUI);
	LambdaEngine::GUIApplication::SetView(m_View);

	PlayerManagerClient::RegisterLocalPlayer(m_Name);
}

void LobbyState::Tick(LambdaEngine::Timestamp delta)
{

}

void LobbyState::FixedTick(LambdaEngine::Timestamp delta)
{

}

bool LobbyState::OnPlayerJoinedEvent(const PlayerJoinedEvent& event)
{
	m_LobbyGUI->AddPlayer(*event.pPlayer);
	return false;
}

bool LobbyState::OnPlayerLeftEvent(const PlayerLeftEvent& event)
{
	m_LobbyGUI->RemovePlayer(*event.pPlayer);
	return false;
}

bool LobbyState::OnPlayerStateUpdatedEvent(const PlayerStateUpdatedEvent& event)
{
	m_LobbyGUI->UpdatePlayerReady(*event.pPlayer);
	return false;
}

bool LobbyState::OnPlayerPingUpdatedEvent(const PlayerPingUpdatedEvent& event)
{
	m_LobbyGUI->UpdatePlayerPing(*event.pPlayer);
	return false;
}

bool LobbyState::OnChatEvent(const ChatEvent& event)
{
	m_LobbyGUI->WriteChatMessage(event);
	return false;
}