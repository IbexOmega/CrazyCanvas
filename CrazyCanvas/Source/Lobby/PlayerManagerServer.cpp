#include "Lobby/PlayerManagerServer.h"

#include "Game/Multiplayer/MultiplayerUtils.h"

#include "Multiplayer/ServerHelper.h"

#include "Events/PlayerEvents.h"
#include "Application/API/Events/EventQueue.h"

using namespace LambdaEngine;

Timestamp PlayerManagerServer::s_Timer;

void PlayerManagerServer::Init()
{
	PlayerManagerBase::Init();

	EventQueue::RegisterEventHandler<PacketReceivedEvent<PacketJoin>>(&PlayerManagerServer::OnPacketJoinReceived);
	EventQueue::RegisterEventHandler<PacketReceivedEvent<PacketLeave>>(&PlayerManagerServer::OnPacketLeaveReceived);
	EventQueue::RegisterEventHandler<ClientDisconnectedEvent>(&PlayerManagerServer::OnClientDisconnected);
	EventQueue::RegisterEventHandler<PacketReceivedEvent<PacketPlayerInfo>>(&PlayerManagerServer::OnPacketPlayerInfoReceived);
}

void PlayerManagerServer::Release()
{
	PlayerManagerBase::Release();

	EventQueue::UnregisterEventHandler<PacketReceivedEvent<PacketJoin>>(&PlayerManagerServer::OnPacketJoinReceived);
	EventQueue::UnregisterEventHandler<PacketReceivedEvent<PacketLeave>>(&PlayerManagerServer::OnPacketLeaveReceived);
	EventQueue::UnregisterEventHandler<ClientDisconnectedEvent>(&PlayerManagerServer::OnClientDisconnected);
	EventQueue::UnregisterEventHandler<PacketReceivedEvent<PacketPlayerInfo>>(&PlayerManagerServer::OnPacketPlayerInfoReceived);
}

void PlayerManagerServer::FixedTick(Timestamp deltaTime)
{
	if (MultiplayerUtils::IsServer())
	{
		static const Timestamp timestep = Timestamp::Seconds(1);

		s_Timer += deltaTime;
		if (s_Timer > timestep)
		{
			s_Timer = 0;

			PacketPlayerInfo packet;
			for (auto& pair : s_Players)
			{
				const Player& player = pair.second;
				packet.UID				= player.m_UID;
				packet.Ping				= player.m_Ping;
				packet.State			= player.m_State;
				packet.Team				= player.m_Team;
				packet.Kills			= player.m_Kills;
				packet.Deaths			= player.m_Deaths;
				packet.FlagsCaptured	= player.m_FlagsCaptured;
				packet.FlagsDefended	= player.m_FlagsDefended;
				ServerHelper::SendBroadcast(packet);
			}
		}
	}
}

bool PlayerManagerServer::OnPacketJoinReceived(const PacketReceivedEvent<PacketJoin>& event)
{
	IClient* pClient = event.pClient;
	Player* pPlayer = HandlePlayerJoined(pClient->GetUID(), event.Packet);
	ServerHelper::SendBroadcast(event.Packet, nullptr, pClient);

	PacketJoin packet;
	for (auto& pair : s_Players)
	{
		const Player& player = pair.second;
		if (&player != pPlayer)
		{
			packet.UID = player.m_UID;
			strcpy(packet.Name, player.GetName().c_str());
			ServerHelper::Send(pClient, packet);
		}
	}

	return true;
}

bool PlayerManagerServer::OnPacketLeaveReceived(const PacketReceivedEvent<PacketLeave>& event)
{
	HandlePlayerLeftServer(event.pClient);
	return true;
}

bool PlayerManagerServer::OnClientDisconnected(const LambdaEngine::ClientDisconnectedEvent& event)
{
	HandlePlayerLeftServer(event.pClient);
	return false;
}

bool PlayerManagerServer::OnPacketPlayerInfoReceived(const PacketReceivedEvent<PacketPlayerInfo>& event)
{
	const PacketPlayerInfo& packet = event.Packet;

	auto pair = s_Players.find(packet.UID);
	if (pair != s_Players.end())
	{
		Player& player			= pair->second;
		player.m_State			= packet.State;

		PlayerInfoUpdatedEvent playerInfoUpdatedEvent(player);
		EventQueue::SendEventImmediate(playerInfoUpdatedEvent);

		PacketPlayerInfo packetNew;
		packetNew.UID			= player.m_UID;
		packetNew.Ping			= player.m_Ping;
		packetNew.State			= player.m_State;
		packetNew.Team			= player.m_Team;
		packetNew.Kills			= player.m_Kills;
		packetNew.Deaths		= player.m_Deaths;
		packetNew.FlagsCaptured = player.m_FlagsCaptured;
		packetNew.FlagsDefended = player.m_FlagsDefended;
		ServerHelper::SendBroadcast(packetNew, nullptr, event.pClient);
	}

	return true;
}

void PlayerManagerServer::HandlePlayerLeftServer(LambdaEngine::IClient* pClient)
{
	HandlePlayerLeft(pClient->GetUID());

	PacketLeave packet;
	packet.UID = pClient->GetUID();
	ServerHelper::SendBroadcast(packet, nullptr, pClient);
}
