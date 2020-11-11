#include "States/ServerState.h"

#include "Memory/API/Malloc.h"

#include "Log/Log.h"

#include "Input/API/Input.h"

#include "Application/API/PlatformMisc.h"
#include "Application/API/CommonApplication.h"
#include "Application/API/PlatformConsole.h"
#include "Application/API/Window.h"
#include "Application/API/Events/EventQueue.h"

#include "Threading/API/Thread.h"

#include "Networking/API/PlatformNetworkUtils.h"

#include "Math/Random.h"

#include <argh/argh.h>

#include "Teams/TeamHelper.h"

#include "Game/Multiplayer/Server/ServerSystem.h"

#include "ECS/ECSCore.h"

#include "World/LevelManager.h"

#include "Match/Match.h"

#include "Multiplayer/Packet/PacketType.h"

#include "ECS/Systems/Match/ServerFlagSystem.h"

#include "Multiplayer/ServerHelper.h"

#include "Lobby/PlayerManagerServer.h"

#include <windows.h>
#include <Lmcons.h>

using namespace LambdaEngine;

ServerState::ServerState(const std::string& clientHostID) :
	m_MultiplayerServer(),
	m_ClientHostID(0)
{
	if (!clientHostID.empty())
		m_ClientHostID = std::stoi(clientHostID);
	else
		m_ClientHostID = Random::Int32(0);

	DWORD length = UNLEN + 1;
	char buffer[UNLEN + 1];
	GetUserNameA(buffer, &length);
	m_ServerName = buffer;
}

ServerState::~ServerState()
{
	EventQueue::UnregisterEventHandler<ServerDiscoveryPreTransmitEvent>(this, &ServerState::OnServerDiscoveryPreTransmit);
	EventQueue::UnregisterEventHandler<KeyPressedEvent>(this, &ServerState::OnKeyPressed);
	EventQueue::UnregisterEventHandler<PacketReceivedEvent<PacketGameSettings>>(this, &ServerState::OnPacketGameSettingsReceived);
}

void ServerState::Init()
{
	EventQueue::RegisterEventHandler<ServerDiscoveryPreTransmitEvent>(this, &ServerState::OnServerDiscoveryPreTransmit);
	EventQueue::RegisterEventHandler<KeyPressedEvent>(this, &ServerState::OnKeyPressed);
	EventQueue::RegisterEventHandler<PacketReceivedEvent<PacketGameSettings>>(this, &ServerState::OnPacketGameSettingsReceived);

	CommonApplication::Get()->GetMainWindow()->SetTitle("Server");
	PlatformConsole::SetTitle("Server Console");

	m_MeshPaintHandler.Init();
	m_MultiplayerServer.InitInternal();

	// Load Match
	{
		const LambdaEngine::TArray<LambdaEngine::SHA256Hash>& levelHashes = LevelManager::GetLevelHashes();

		MatchDescription matchDescription =
		{
			.LevelHash = levelHashes[0]
		};
		Match::CreateMatch(&matchDescription);
	}

	ServerSystem::GetInstance().Start();
}

bool ServerState::OnKeyPressed(const KeyPressedEvent& event)
{
	UNREFERENCED_VARIABLE(event);
	return false;
}

bool ServerState::OnServerDiscoveryPreTransmit(const LambdaEngine::ServerDiscoveryPreTransmitEvent& event)
{
	BinaryEncoder* pEncoder = event.pEncoder;
	ServerBase* pServer = event.pServer;

	pEncoder->WriteUInt8(pServer->GetClientCount());
	pEncoder->WriteString(m_ServerName);
	pEncoder->WriteString("Map Name");
	pEncoder->WriteInt32(m_ClientHostID);

	return true;
}

void ServerState::Tick(Timestamp delta)
{
	m_MultiplayerServer.TickMainThreadInternal(delta);
}

void ServerState::FixedTick(LambdaEngine::Timestamp delta)
{
	m_MultiplayerServer.FixedTickMainThreadInternal(delta);
}

bool ServerState::OnPacketGameSettingsReceived(const PacketReceivedEvent<PacketGameSettings>& event)
{	
	const PacketGameSettings& packet = event.Packet;

	if (PlayerManagerServer::HasPlayerAuthority(event.pClient))
	{
		LOG_INFO("Configuring Server With The Following Settings:");
		LOG_INFO("Players: %hhu", packet.Players);
		LOG_INFO("MapID: %hhu", packet.MapID);

		ServerHelper::SetMaxClients(packet.Players);
	}
	else
	{
		LOG_ERROR("Unauthorised Client tried to exectute a server command!");
	}

	return true;
}