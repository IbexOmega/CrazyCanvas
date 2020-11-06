#include "Lobby/PlayerManagerClient.h"

#include "Multiplayer/ClientHelper.h"

#include "Game/Multiplayer/Client/ClientSystem.h"

#include "Game/Multiplayer/MultiplayerUtils.h"

#include "Application/API/Events/EventQueue.h"

#include "Events/PlayerEvents.h"

using namespace LambdaEngine;

const Player* PlayerManagerClient::GetPlayerLocal()
{
	ASSERT(!MultiplayerUtils::IsServer());
	return GetPlayer(ClientSystem::GetInstance().GetClient()->GetUID());
}

Player* PlayerManagerClient::GetPlayerLocalNoConst()
{
	ASSERT(!MultiplayerUtils::IsServer());
	return GetPlayerNoConst(ClientSystem::GetInstance().GetClient()->GetUID());
}

void PlayerManagerClient::RegisterLocalPlayer(const String& name)
{
	IClient* pClient = ClientSystem::GetInstance().GetClient();

	ASSERT(s_Players.empty());
	ASSERT(pClient->IsConnected());
	ASSERT(!MultiplayerUtils::IsServer());

	PacketJoin packet;
	strcpy(packet.Name, name.c_str());
	packet.UID = pClient->GetUID();

	HandlePlayerJoined(packet.UID, packet);

	ClientHelper::Send(packet);
}

void PlayerManagerClient::SetLocalPlayerReady(bool ready)
{
	Player* pPlayer = GetPlayerLocalNoConst();
	if (pPlayer)
	{
		EPlayerState currentState = pPlayer->m_State;
		if (currentState == PLAYER_STATE_READY || currentState == PLAYER_STATE_LOBBY)
		{
			PacketPlayerInfo packet;
			UpdatePacketFromPlayer(&packet, pPlayer);
			packet.State = ready ? PLAYER_STATE_READY : PLAYER_STATE_LOBBY;

			if(UpdatePlayerFromPacket(pPlayer, &packet))
			{
				ClientHelper::Send(packet);

				PlayerInfoUpdatedEvent playerInfoUpdatedEvent(pPlayer);
				EventQueue::SendEventImmediate(playerInfoUpdatedEvent);
			}
		}
	}
}

void PlayerManagerClient::Init()
{
	PlayerManagerBase::Init();

	EventQueue::RegisterEventHandler<PacketReceivedEvent<PacketJoin>>(&PlayerManagerClient::OnPacketJoinReceived);
	EventQueue::RegisterEventHandler<PacketReceivedEvent<PacketLeave>>(&PlayerManagerClient::OnPacketLeaveReceived);
	EventQueue::RegisterEventHandler<ClientDisconnectedEvent>(&PlayerManagerClient::OnClientDisconnected);
	EventQueue::RegisterEventHandler<PacketReceivedEvent<PacketPlayerInfo>>(&PlayerManagerClient::OnPacketPlayerInfoReceived);
}

void PlayerManagerClient::Release()
{
	PlayerManagerBase::Release();

	EventQueue::UnregisterEventHandler<PacketReceivedEvent<PacketJoin>>(&PlayerManagerClient::OnPacketJoinReceived);
	EventQueue::UnregisterEventHandler<PacketReceivedEvent<PacketLeave>>(&PlayerManagerClient::OnPacketLeaveReceived);
	EventQueue::UnregisterEventHandler<ClientDisconnectedEvent>(&PlayerManagerClient::OnClientDisconnected);
	EventQueue::UnregisterEventHandler<PacketReceivedEvent<PacketPlayerInfo>>(&PlayerManagerClient::OnPacketPlayerInfoReceived);
}

bool PlayerManagerClient::OnPacketJoinReceived(const PacketReceivedEvent<PacketJoin>& event)
{
	const PacketJoin& packet = event.Packet;
	HandlePlayerJoined(packet.UID, packet);
	return true;
}

bool PlayerManagerClient::OnPacketLeaveReceived(const PacketReceivedEvent<PacketLeave>& event)
{
	const PacketLeave& packet = event.Packet;
	HandlePlayerLeft(packet.UID);
	return true;
}

bool PlayerManagerClient::OnClientDisconnected(const LambdaEngine::ClientDisconnectedEvent& event)
{
	IClient* pClient = event.pClient;
	HandlePlayerLeft(pClient->GetUID());
	return true;
}

bool PlayerManagerClient::OnPacketPlayerInfoReceived(const PacketReceivedEvent<PacketPlayerInfo>& event)
{
	const PacketPlayerInfo& packet = event.Packet;

	Player* pPlayer = GetPlayerNoConst(packet.UID);

	if (pPlayer)
	{
		if (UpdatePlayerFromPacket(pPlayer, &packet))
		{
			PlayerInfoUpdatedEvent playerInfoUpdatedEvent(pPlayer);
			EventQueue::SendEventImmediate(playerInfoUpdatedEvent);
		}
	}

	return false;
}