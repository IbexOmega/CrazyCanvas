#include "Lobby/PlayerManagerClient.h"

#include "Networking/API/NetworkDebugger.h"

#include "Multiplayer/ClientHelper.h"

#include "Game/Multiplayer/Client/ClientSystem.h"

#include "Game/Multiplayer/MultiplayerUtils.h"

#include "Application/API/Events/EventQueue.h"

#include "Multiplayer/Packet/PacketPlayerState.h"
#include "Multiplayer/Packet/PacketPlayerReady.h"

#include "Events/PlayerEvents.h"

using namespace LambdaEngine;

void PlayerManagerClient::Init()
{
	PlayerManagerBase::Init();

	EventQueue::RegisterEventHandler<PacketReceivedEvent<PacketJoin>>(&PlayerManagerClient::OnPacketJoinReceived);
	EventQueue::RegisterEventHandler<PacketReceivedEvent<PacketLeave>>(&PlayerManagerClient::OnPacketLeaveReceived);
	EventQueue::RegisterEventHandler<ClientDisconnectedEvent>(&PlayerManagerClient::OnClientDisconnected);
	EventQueue::RegisterEventHandler<PacketReceivedEvent<PacketPlayerScore>>(&PlayerManagerClient::OnPacketPlayerScoreReceived);
	EventQueue::RegisterEventHandler<PacketReceivedEvent<PacketPlayerState>>(&PlayerManagerClient::OnPacketPlayerStateReceived);
	EventQueue::RegisterEventHandler<PacketReceivedEvent<PacketPlayerReady>>(&PlayerManagerClient::OnPacketPlayerReadyReceived);
	EventQueue::RegisterEventHandler<PacketReceivedEvent<PacketPlayerAliveChanged>>(&PlayerManagerClient::OnPacketPlayerAliveChangedReceived);
	EventQueue::RegisterEventHandler<PacketReceivedEvent<PacketPlayerPing>>(&PlayerManagerClient::OnPacketPlayerPingReceived);
	EventQueue::RegisterEventHandler<PacketReceivedEvent<PacketPlayerHost>>(&PlayerManagerClient::OnPacketPlayerHostReceived);
}

void PlayerManagerClient::Release()
{
	PlayerManagerBase::Release();

	EventQueue::UnregisterEventHandler<PacketReceivedEvent<PacketJoin>>(&PlayerManagerClient::OnPacketJoinReceived);
	EventQueue::UnregisterEventHandler<PacketReceivedEvent<PacketLeave>>(&PlayerManagerClient::OnPacketLeaveReceived);
	EventQueue::UnregisterEventHandler<ClientDisconnectedEvent>(&PlayerManagerClient::OnClientDisconnected);
	EventQueue::UnregisterEventHandler<PacketReceivedEvent<PacketPlayerScore>>(&PlayerManagerClient::OnPacketPlayerScoreReceived);
	EventQueue::UnregisterEventHandler<PacketReceivedEvent<PacketPlayerState>>(&PlayerManagerClient::OnPacketPlayerStateReceived);
	EventQueue::UnregisterEventHandler<PacketReceivedEvent<PacketPlayerReady>>(&PlayerManagerClient::OnPacketPlayerReadyReceived);
	EventQueue::UnregisterEventHandler<PacketReceivedEvent<PacketPlayerAliveChanged>>(&PlayerManagerClient::OnPacketPlayerAliveChangedReceived);
	EventQueue::UnregisterEventHandler<PacketReceivedEvent<PacketPlayerPing>>(&PlayerManagerClient::OnPacketPlayerPingReceived);
	EventQueue::UnregisterEventHandler<PacketReceivedEvent<PacketPlayerHost>>(&PlayerManagerClient::OnPacketPlayerHostReceived);
}

void PlayerManagerClient::Reset()
{
	PlayerManagerBase::Reset();
}

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

void PlayerManagerClient::RegisterLocalPlayer(const String& name, bool isHost)
{
	IClient* pClient = ClientSystem::GetInstance().GetClient();

	ASSERT(s_Players.empty());
	ASSERT(!MultiplayerUtils::IsServer());

	PacketJoin packet;
	strcpy(packet.Name, name.c_str());
	packet.UID = pClient->GetUID();

	Player* pPlayer = HandlePlayerJoined(packet.UID, packet);
	pPlayer->m_IsHost = isHost;

	if (name == "Singleplayer")
	{
		pPlayer->m_Team = 1;
	}

	ClientHelper::Send(packet);
	NetworkDebugger::RegisterClientName(pClient, name);

	if (isHost)
	{
		PlayerHostUpdatedEvent event(pPlayer);
		EventQueue::SendEventImmediate(event);
	}
}

void PlayerManagerClient::SetLocalPlayerReady(bool ready)
{
	ASSERT(!MultiplayerUtils::IsServer());

	Player* pPlayer = GetPlayerLocalNoConst();
	if (pPlayer)
	{
		ASSERT_MSG(pPlayer->GetState() == GAME_STATE_LOBBY, "Player not in LOBBY state!");

		if (pPlayer->IsHost())
		{
			pPlayer->m_State = GAME_STATE_SETUP;

			PacketPlayerState packet;
			packet.State = pPlayer->m_State;

			ClientHelper::Send(packet);

			PlayerStateUpdatedEvent event(pPlayer);
			EventQueue::SendEventImmediate(event);
		}
		else if (pPlayer->m_IsReady != ready)
		{
			pPlayer->m_IsReady = ready;

			PacketPlayerReady packet;
			packet.IsReady = pPlayer->m_IsReady;

			ClientHelper::Send(packet);

			PlayerReadyUpdatedEvent playerReadyUpdatedEvent(pPlayer);
			EventQueue::SendEventImmediate(playerReadyUpdatedEvent);
		}
	}
}

void PlayerManagerClient::SetLocalPlayerStateLoading()
{
	ASSERT(!MultiplayerUtils::IsServer());

	Player* pPlayer = GetPlayerLocalNoConst();
	if (pPlayer)
	{
		ASSERT_MSG(pPlayer->GetState() == GAME_STATE_SETUP, "Player not in SETUP state!");

		ClientHelper::SetTimeout(Timestamp::Seconds(15));

		pPlayer->m_State = GAME_STATE_LOADING;

		PacketPlayerState packet;
		packet.State = pPlayer->m_State;

		ClientHelper::Send(packet);

		PlayerStateUpdatedEvent event(pPlayer);
		EventQueue::SendEventImmediate(event);
	}
}

void PlayerManagerClient::SetLocalPlayerStateLoaded()
{
	ASSERT(!MultiplayerUtils::IsServer());

	Player* pPlayer = GetPlayerLocalNoConst();
	if (pPlayer)
	{
		ASSERT_MSG(pPlayer->GetState() == GAME_STATE_LOADING, "Player not in LOADING state!");

		ClientHelper::ResetTimeout();

		pPlayer->m_State = GAME_STATE_LOADED;

		PacketPlayerState packet;
		packet.State = pPlayer->m_State;

		ClientHelper::Send(packet);

		PlayerStateUpdatedEvent event(pPlayer);
		EventQueue::SendEventImmediate(event);
	}
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

bool PlayerManagerClient::OnPacketPlayerScoreReceived(const PacketReceivedEvent<PacketPlayerScore>& event)
{
	const PacketPlayerScore& packet = event.Packet;

	Player* pPlayer = GetPlayerNoConst(packet.UID);

	if (pPlayer)
	{
		bool changed = false;
		if (pPlayer->m_Team != packet.Team)
		{
			changed = true;
			pPlayer->m_Team = packet.Team;
		}
		if (pPlayer->m_Kills != packet.Kills)
		{
			changed = true;
			pPlayer->m_Kills = packet.Kills;
		}
		if (pPlayer->m_Deaths != packet.Deaths)
		{
			changed = true;
			pPlayer->m_Deaths = packet.Deaths;
		}
		if (pPlayer->m_FlagsCaptured != packet.FlagsCaptured)
		{
			changed = true;
			pPlayer->m_FlagsCaptured = packet.FlagsCaptured;
		}
		if (pPlayer->m_FlagsDefended != packet.FlagsDefended)
		{
			changed = true;
			pPlayer->m_FlagsDefended = packet.FlagsDefended;
		}

		if (changed)
		{
			PlayerScoreUpdatedEvent playerScoreUpdatedEvent(pPlayer);
			EventQueue::SendEventImmediate(playerScoreUpdatedEvent);
		}
	}

	return true;
}

bool PlayerManagerClient::OnPacketPlayerStateReceived(const PacketReceivedEvent<PacketPlayerState>& event)
{
	const PacketPlayerState& packet = event.Packet;

	Player* pPlayer = GetPlayerNoConst(packet.UID);

	if (pPlayer)
	{
		if (pPlayer->m_State != packet.State)
		{
			pPlayer->m_State = packet.State;

			PlayerStateUpdatedEvent playerStateUpdatedEvent(pPlayer);
			EventQueue::SendEventImmediate(playerStateUpdatedEvent);
		}
	}
	return true;
}

bool PlayerManagerClient::OnPacketPlayerReadyReceived(const PacketReceivedEvent<PacketPlayerReady>& event)
{
	const PacketPlayerReady& packet = event.Packet;

	Player* pPlayer = GetPlayerNoConst(packet.UID);

	if (pPlayer)
	{
		if (pPlayer->m_IsReady != packet.IsReady)
		{
			pPlayer->m_IsReady = packet.IsReady;

			PlayerReadyUpdatedEvent playerReadyUpdatedEvent(pPlayer);
			EventQueue::SendEventImmediate(playerReadyUpdatedEvent);
		}
	}
	return true;
}

bool PlayerManagerClient::OnPacketPlayerAliveChangedReceived(const PacketReceivedEvent<PacketPlayerAliveChanged>& event)
{
	const PacketPlayerAliveChanged& packet = event.Packet;

	Player* pPlayer = GetPlayerNoConst(packet.UID);

	if (pPlayer)
	{
		if (pPlayer->m_Deaths != packet.Deaths)
		{
			pPlayer->m_Deaths = packet.Deaths;

			PlayerScoreUpdatedEvent playerScoreUpdatedEvent(pPlayer);
			EventQueue::SendEventImmediate(playerScoreUpdatedEvent);
		}
		if (pPlayer->m_IsDead != packet.IsDead)
		{
			pPlayer->m_IsDead = packet.IsDead;
			if (pPlayer->m_IsDead)
			{
				LOG_INFO("CLIENT PLAYER DIED Entity=%u", pPlayer->GetEntity());
			}

			PlayerAliveUpdatedEvent playerAliveUpdatedEvent(pPlayer, GetPlayerNoConst(packet.KillerUID));
			EventQueue::SendEventImmediate(playerAliveUpdatedEvent);
		}
	}

	return true;
}

bool PlayerManagerClient::OnPacketPlayerHostReceived(const PacketReceivedEvent<PacketPlayerHost>& event)
{
	const PacketPlayerHost& packet = event.Packet;

	Player* pPlayer = GetPlayerNoConst(packet.UID);

	if (pPlayer)
	{
		if (!pPlayer->m_IsHost)
		{
			for (auto& pair : s_Players)
			{
				Player& player = pair.second;
				if (player.IsHost())
				{
					player.m_IsHost = false;
					PlayerHostUpdatedEvent playerHostUpdatedEvent(pPlayer);
					EventQueue::SendEventImmediate(playerHostUpdatedEvent);
				}
			}

			pPlayer->m_IsHost = true;

			PlayerHostUpdatedEvent playerHostUpdatedEvent(pPlayer);
			EventQueue::SendEventImmediate(playerHostUpdatedEvent);
		}
	}
	return true;
}

bool PlayerManagerClient::OnPacketPlayerPingReceived(const PacketReceivedEvent<PacketPlayerPing>& event)
{
	const PacketPlayerPing& packet = event.Packet;

	Player* pPlayer = GetPlayerNoConst(packet.UID);

	if (pPlayer)
	{
		if (pPlayer->m_Ping != packet.Ping)
		{
			pPlayer->m_Ping = packet.Ping;

			PlayerPingUpdatedEvent playerPingUpdatedEvent(pPlayer);
			EventQueue::SendEventImmediate(playerPingUpdatedEvent);
		}
	}
	return true;
}