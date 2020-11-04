#include "Lobby/PlayerManager.h"

#include "Game/Multiplayer/MultiplayerUtils.h"

#include "Game/Multiplayer/Client/ClientSystem.h"

#include "Application/API/Events/EventQueue.h"

#include "Lobby/PlayerEvents.h"

#include "Multiplayer/ServerHelper.h"
#include "Multiplayer/ClientHelper.h"

using namespace LambdaEngine;

THashTable<uint64, Player> PlayerManager::m_Players;

void PlayerManager::Init()
{
	EventQueue::RegisterEventHandler<PacketReceivedEvent<PacketJoin>>(&PlayerManager::OnPacketJoinReceived);
	EventQueue::RegisterEventHandler<PacketReceivedEvent<PacketLeave>>(&PlayerManager::OnPacketLeaveReceived);
	EventQueue::RegisterEventHandler<ClientDisconnectedEvent>(&PlayerManager::OnClientDisconnected);
}

void PlayerManager::Release()
{
	EventQueue::UnregisterEventHandler<PacketReceivedEvent<PacketJoin>>(&PlayerManager::OnPacketJoinReceived);
	EventQueue::UnregisterEventHandler<PacketReceivedEvent<PacketLeave>>(&PlayerManager::OnPacketLeaveReceived);
	EventQueue::UnregisterEventHandler<ClientDisconnectedEvent>(&PlayerManager::OnClientDisconnected);
}

const Player* PlayerManager::GetPlayer(uint64 uid)
{
    auto pair = m_Players.find(uid);
    return pair == m_Players.end() ? nullptr : &pair->second;
}

const Player* PlayerManager::GetPlayer(LambdaEngine::IClient* pClient)
{
    return GetPlayer(pClient->GetUID());
}

const Player* PlayerManager::GetPlayerLocal()
{
	return GetPlayer(ClientSystem::GetInstance().GetClient()->GetUID());
}

void PlayerManager::RegisterLocalPlayer(const String& name, LambdaEngine::IClient* pClient)
{
	ASSERT(m_Players.empty());
	ASSERT(pClient->IsConnected());
	ASSERT(!MultiplayerUtils::IsServer());

	Player player;
	player.m_Name = name;
	player.m_UID = pClient->GetUID();

	HandlePlayerJoined(pClient, player);
}

bool PlayerManager::OnPacketJoinReceived(const PacketReceivedEvent<PacketJoin>& event)
{
	const PacketJoin& packet = event.Packet;
	IClient* pClient = event.pClient;
	uint64 uid = packet.UID;

	if (MultiplayerUtils::IsServer())
		uid = pClient->GetUID();

	auto pair = m_Players.find(uid);
	if (pair == m_Players.end())
	{
		Player player;
		player.m_Name = packet.Name;
		player.m_UID = uid;

		HandlePlayerJoined(pClient, player);
	}
	else
	{
		pair->second.m_Name = packet.Name;
	}

	return true;
}

bool PlayerManager::OnPacketLeaveReceived(const PacketReceivedEvent<PacketLeave>& event)
{
	const PacketLeave& packet = event.Packet;
	IClient* pClient = event.pClient;
	uint64 uid = packet.UID;

	if (MultiplayerUtils::IsServer())
		uid = pClient->GetUID();

	HandlePlayerLeft(pClient, uid);
	
	return true;
}

bool PlayerManager::OnClientDisconnected(const LambdaEngine::ClientDisconnectedEvent& event)
{
	IClient* pClient = event.pClient;
	HandlePlayerLeft(pClient, pClient->GetUID());
	return true;
}

void PlayerManager::HandlePlayerJoined(IClient* pClient, const Player& player)
{
	m_Players.insert({ player.GetUID(), player });

	LOG_INFO("Player [%s] joined! [%llu]", player.GetName().c_str(), player.GetUID());

	PlayerJoinedEvent newEvent(player);
	EventQueue::SendEventImmediate(newEvent);

	PacketJoin packet;
	packet.UID = player.m_UID;
	strcpy(packet.Name, player.GetName().c_str());

	if (MultiplayerUtils::IsServer())
	{
		for (auto& pair : m_Players)
		{
			if (pair.second != player)
			{
				const Player& playerOther = pair.second;
				PacketJoin packetOther;
				packetOther.UID = playerOther.m_UID;
				strcpy(packetOther.Name, playerOther.GetName().c_str());
				ServerHelper::Send(pClient, packetOther);
			}
		}
		ServerHelper::SendBroadcast(packet, nullptr, pClient);
	}
	else
	{
		ClientHelper::Send(packet);
	}
}

void PlayerManager::HandlePlayerLeft(IClient* pClientToExclude, uint64 uid)
{
	auto pair = m_Players.find(uid);
	if (pair != m_Players.end())
	{
		const Player& player = pair->second;

		LOG_INFO("Player [%s] left! [%llu]", player.GetName().c_str(), player.GetUID());

		PlayerLeftEvent event(player);
		EventQueue::SendEventImmediate(event);

		if (MultiplayerUtils::IsServer())
		{
			PacketLeave packet;
			packet.UID = uid;
			ServerHelper::SendBroadcast(packet, nullptr, pClientToExclude);
		}

		m_Players.erase(uid);
	}
}