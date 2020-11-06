#include "States/LobbyState.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"

#include "Chat/ChatManager.h"

#include "Lobby/PlayerManagerClient.h"

#include "Application/API/Events/EventQueue.h"

#include "Multiplayer/Packet/PacketConfigureServer.h"
#include "Multiplayer/ServerHostHelper.h"
#include "Multiplayer/ClientHelper.h"

using namespace LambdaEngine;

LobbyState::~LobbyState()
{
	EventQueue::UnregisterEventHandler<PlayerJoinedEvent>(this, &LobbyState::OnPlayerJoinedEvent);
	EventQueue::UnregisterEventHandler<PlayerLeftEvent>(this, &LobbyState::OnPlayerLeftEvent);
	EventQueue::UnregisterEventHandler<PlayerInfoUpdatedEvent>(this, &LobbyState::OnPlayerInfoUpdatedEvent);
}

void LobbyState::Init()
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

	/*m_LobbyGUI = *new LobbyGUI("Lobby.xaml");
	m_View = Noesis::GUI::CreateView(m_LobbyGUI);
	LambdaEngine::GUIApplication::SetView(m_View);*/

	ChatManager::SendChatMessage("This is a chat message");
	PlayerManagerClient::SetLocalPlayerReady(true);
}

void LobbyState::Tick(LambdaEngine::Timestamp delta)
{

}

void LobbyState::FixedTick(LambdaEngine::Timestamp delta)
{

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
}

void LobbyState::SendServerConfiguration()
{
	PacketConfigureServer packet;
	packet.AuthenticationID	= ServerHostHelper::GetAuthenticationHostID();
	/*packet.MapID			= m_HostGameDesc.MapNumber;
	packet.Players			= m_HostGameDesc.PlayersNumber;*/

	ClientHelper::Send(packet);
}