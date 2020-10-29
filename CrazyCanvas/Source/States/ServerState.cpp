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

#include <windows.h>
#include <Lmcons.h>

using namespace LambdaEngine;

ServerState::ServerState(const std::string& clientHostID, const std::string& authenticationID) :
	m_MultiplayerServer()
{
	ServerHostHelper::SetAuthenticationID(std::stoi(authenticationID)); //ID server checks to authenticate client with higher authorities
	ServerHostHelper::SetClientHostID(std::stoi(clientHostID)); // ID client checks to see if it were the creater of server
}

ServerState::ServerState() : 
	m_MultiplayerServer()
{
	m_ServerName.reserve(UNLEN + 1);
	DWORD length = m_ServerName.capacity();
	GetUserNameA(m_ServerName.data(), &length);
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
	EventQueue::RegisterEventHandler<PacketReceivedEvent<PacketHostServer>>(this, &ServerState::OnPacketHostServerReceived);

	CommonApplication::Get()->GetMainWindow()->SetTitle("Server");
	PlatformConsole::SetTitle("Server Console");

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

bool ServerState::OnPacketHostServerReceived(const PacketReceivedEvent<PacketHostServer>& event)
{	
	const PacketHostServer& packet = event.Packet;

	int8 nrOfPlayers = packet.PlayersNumber;
	int8 mapNr = packet.MapNumber;
	int32 authenticationID = packet.AuthenticationID;

	LOG_ERROR("Starting Server With The Following Information:");
	LOG_ERROR("NR OF PLAYERS %d", nrOfPlayers);
	LOG_ERROR("MAP NR %d", mapNr);
	LOG_ERROR("receivedHostID: %d", authenticationID);
	LOG_ERROR("LocalHostID: %d", ServerHostHelper::GetAuthenticationHostID());

	return false;
}
