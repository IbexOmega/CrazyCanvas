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
#include "Multiplayer/ServerHostHelper.h"

#include "ECS/Systems/Match/ServerFlagSystem.h"

using namespace LambdaEngine;

ServerState::ServerState(const std::string& serverHostID, const std::string& clientHostID) :
	m_MultiplayerServer()
{
	ServerHostHelper::SetAuthenticationID(std::stoi(clientHostID));
	ServerHostHelper::SetClientHostID(std::stoi(serverHostID));
}

ServerState::ServerState() : 
	m_MultiplayerServer()
{

}

ServerState::~ServerState()
{
	EventQueue::UnregisterEventHandler<ServerDiscoveryPreTransmitEvent>(this, &ServerState::OnServerDiscoveryPreTransmit);
	EventQueue::UnregisterEventHandler<KeyPressedEvent>(this, &ServerState::OnKeyPressed);
}

void ServerState::Init()
{
	ServerSystem::GetInstance();

	EventQueue::RegisterEventHandler<ServerDiscoveryPreTransmitEvent>(this, &ServerState::OnServerDiscoveryPreTransmit);
	EventQueue::RegisterEventHandler<KeyPressedEvent>(this, &ServerState::OnKeyPressed);
	EventQueue::RegisterEventHandler<PacketReceivedEvent>(this, &ServerState::OnPacketReceived);

	CommonApplication::Get()->GetMainWindow()->SetTitle("Server");
	PlatformConsole::SetTitle("Server Console");

	m_MultiplayerServer.InitInternal();

	m_ServerName = "Crazy Canvas Server";

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
	pEncoder->WriteInt32(ServerHostHelper::GetClientHostID());

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

bool ServerState::OnPacketReceived(const LambdaEngine::PacketReceivedEvent& event)
{
	NetworkSegment* pPacket = event.pPacket;

	if (pPacket->GetType() == PacketType::HOST_SERVER)
	{
		BinaryDecoder decoder(pPacket);
		int8 nrOfPlayers = decoder.ReadInt8();
		int8 mapNr = decoder.ReadInt8();
		int32 clientHostID = decoder.ReadInt32();

		LOG_ERROR("Starting Server With The Following Information:");
		LOG_ERROR("NR OF PLAYERS %d", nrOfPlayers);
		LOG_ERROR("MAP NR %d", mapNr);
		LOG_ERROR("receivedHostID: %d", clientHostID);
		LOG_ERROR("LocalHostID: %d", ServerHostHelper::GetAuthenticationHostID());
	}
	return false;
}
