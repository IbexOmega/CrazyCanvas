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
				UpdatePacketFromPlayer(&packet, &pair.second);
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

		PlayerInfoUpdatedEvent playerInfoUpdatedEvent(&player);
		EventQueue::SendEventImmediate(playerInfoUpdatedEvent);

		PacketPlayerInfo packetNew;
		UpdatePacketFromPlayer(&packetNew, &player);
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
