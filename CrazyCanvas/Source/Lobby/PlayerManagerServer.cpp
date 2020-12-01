#include "Lobby/PlayerManagerServer.h"

#include "Networking/API/NetworkDebugger.h"

#include "Game/Multiplayer/MultiplayerUtils.h"

#include "Multiplayer/ServerHelper.h"
#include "Multiplayer/Packet/PacketPlayerAliveChanged.h"
#include "Multiplayer/Packet/PacketPlayerHost.h"
#include "Multiplayer/Packet/PacketPlayerPing.h"

#include "ECS/Systems/Player/HealthSystemServer.h"

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
	EventQueue::RegisterEventHandler<PacketReceivedEvent<PacketPlayerState>>(&PlayerManagerServer::OnPacketPlayerStateReceived);
	EventQueue::RegisterEventHandler<PacketReceivedEvent<PacketPlayerReady>>(&PlayerManagerServer::OnPacketPlayerReadyReceived);
}

void PlayerManagerServer::Release()
{
	PlayerManagerBase::Release();

	EventQueue::UnregisterEventHandler<PacketReceivedEvent<PacketJoin>>(&PlayerManagerServer::OnPacketJoinReceived);
	EventQueue::UnregisterEventHandler<PacketReceivedEvent<PacketLeave>>(&PlayerManagerServer::OnPacketLeaveReceived);
	EventQueue::UnregisterEventHandler<ClientDisconnectedEvent>(&PlayerManagerServer::OnClientDisconnected);
	EventQueue::UnregisterEventHandler<PacketReceivedEvent<PacketPlayerState>>(&PlayerManagerServer::OnPacketPlayerStateReceived);
	EventQueue::UnregisterEventHandler<PacketReceivedEvent<PacketPlayerReady>>(&PlayerManagerServer::OnPacketPlayerReadyReceived);
}

void PlayerManagerServer::Reset()
{
	PlayerManagerBase::Reset();
}

void PlayerManagerServer::FixedTick(Timestamp deltaTime)
{
	static const Timestamp timestep = Timestamp::Seconds(1);

	s_Timer += deltaTime;
	if (s_Timer > timestep)
	{
		s_Timer = 0;

		const ClientMap& clients = ServerSystem::GetInstance().GetServer()->GetClients();
		for (auto& pair : clients)
		{
			IClient* pClient = pair.second;
			Player* pPlayer = GetPlayerNoConst(pClient->GetUID());
			if (pPlayer)
			{
				pPlayer->m_Ping = (uint16)pClient->GetStatistics()->GetPing();
				PacketPlayerPing packet;
				packet.Ping = pPlayer->m_Ping;
				packet.UID	= pPlayer->m_UID;
				ServerHelper::SendBroadcast(packet);
			}
		}
	}
}

bool PlayerManagerServer::OnPacketJoinReceived(const PacketReceivedEvent<PacketJoin>& event)
{
	PacketJoin packet = event.Packet;
	IClient* pClient = event.pClient;
	Player* pPlayer = HandlePlayerJoined(pClient->GetUID(), event.Packet);

	packet.UID = pClient->GetUID();

	AutoSelectTeam(pPlayer);

	ServerHelper::SendBroadcast(packet, nullptr, pClient);

	PacketPlayerScore packetPlayerScore;
	FillPacketPlayerScore(&packetPlayerScore, pPlayer);
	ServerHelper::SendBroadcast(packetPlayerScore);

	Player* hostPlayer = nullptr;
	for (auto& pair : s_Players)
	{
		if (pair.second.IsHost())
			hostPlayer = &pair.second;
	}

	for (auto& pair : s_Players)
	{
		const Player& player = pair.second;
		if (&player != pPlayer)
		{
			PacketJoin packetJoin;
			packetJoin.UID = player.m_UID;
			strcpy(packetJoin.Name, player.GetName().c_str());
			ServerHelper::Send(pClient, packetJoin);

			if (hostPlayer)
			{
				PacketPlayerHost packetPlayerHost;
				packetPlayerHost.UID = hostPlayer->GetUID();
				ServerHelper::Send(pClient, packetPlayerHost);
			}

			PacketPlayerReady packetPlayerReady;
			packetPlayerReady.UID		= player.m_UID;
			packetPlayerReady.IsReady	= player.IsReady();
			ServerHelper::Send(pClient, packetPlayerReady);

			PacketPlayerScore packetPlayerScore2;
			FillPacketPlayerScore(&packetPlayerScore2, &player);
			ServerHelper::Send(pClient, packetPlayerScore2);
		}
	}

	if (s_Players.size() == 1)
	{
		SetPlayerHost(pPlayer);
	}

	NetworkDebugger::RegisterClientName(pClient, pPlayer->GetName());

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

bool PlayerManagerServer::OnPacketPlayerStateReceived(const PacketReceivedEvent<PacketPlayerState>& event)
{
	PacketPlayerState packet = event.Packet;
	IClient* pClient = event.pClient;
	packet.UID = pClient->GetUID();

	auto iterator = s_Players.find(packet.UID);
	if (iterator != s_Players.end())
	{
		Player& player = iterator->second;

		if (packet.State == GAME_STATE_LOADING || packet.State == GAME_STATE_LOADED)
		{
			if (player.m_State != packet.State)
			{
				player.m_State = packet.State;

				ServerHelper::SendBroadcast(packet, nullptr, pClient);

				PlayerStateUpdatedEvent playerStateUpdatedEvent(&player);
				EventQueue::SendEventImmediate(playerStateUpdatedEvent);
			}
		}
		else if (packet.State == GAME_STATE_SETUP)
		{
			if (player.IsHost())
			{
				ServerHelper::SetIgnoreNewClients(true);

				for (auto& pair : s_Players)
				{
					Player& p = pair.second;
					if (p != player)
					{
						p.m_State = packet.State;
						packet.UID = p.m_UID;

						ServerHelper::SendBroadcast(packet);

						PlayerStateUpdatedEvent playerStateUpdatedEvent(&player);
						EventQueue::SendEventImmediate(playerStateUpdatedEvent);
					}
				}
			}
		}
	}

	return true;
}

bool PlayerManagerServer::OnPacketPlayerReadyReceived(const PacketReceivedEvent<PacketPlayerReady>& event)
{
	PacketPlayerReady packet = event.Packet;
	IClient* pClient = event.pClient;
	packet.UID = pClient->GetUID();

	auto pair = s_Players.find(packet.UID);
	if (pair != s_Players.end())
	{
		Player& player = pair->second;

		if (player.m_IsReady != packet.IsReady)
		{
			player.m_IsReady = packet.IsReady;

			ServerHelper::SendBroadcast(packet, nullptr, pClient);

			PlayerReadyUpdatedEvent playerReadyUpdatedEvent(&player);
			EventQueue::SendEventImmediate(playerReadyUpdatedEvent);
		}
	}
	return true;
}

void PlayerManagerServer::HandlePlayerLeftServer(LambdaEngine::IClient* pClient)
{
	bool wasHost = HandlePlayerLeft(pClient->GetUID());

	PacketLeave packet;
	packet.UID = pClient->GetUID();
	ServerHelper::SendBroadcast(packet, nullptr, pClient);

	if (wasHost && !s_Players.empty())
	{
		SetPlayerHost(&(s_Players.begin()->second));
	}
}

bool PlayerManagerServer::HasPlayerAuthority(const IClient* pClient)
{
	const Player* pPlayer = GetPlayer(pClient);
	return pPlayer != nullptr && pPlayer->IsHost();
}

void PlayerManagerServer::SetPlayerState(const Player* pPlayer, EGameState state)
{
	if (pPlayer->m_State != state)
	{
		Player* pPl = const_cast<Player*>(pPlayer);
		pPl->m_State = state;

		PacketPlayerState packet;
		packet.UID		= pPl->m_UID;
		packet.State	= pPl->m_State;
		ServerHelper::SendBroadcast(packet);

		PlayerStateUpdatedEvent event(pPlayer);
		EventQueue::SendEventImmediate(event);
	}
}

void PlayerManagerServer::SetPlayerAlive(const Player* pPlayer, bool alive, const Player* pPlayerKiller)
{
	if (pPlayer->m_IsDead == alive)
	{
		Player* pPl = const_cast<Player*>(pPlayer);
		pPl->m_IsDead = !alive;

		if (pPl->m_IsDead)
			pPl->m_Deaths++;
		else
			HealthSystemServer::ResetHealth(pPl->GetEntity());

		PacketPlayerAliveChanged packet;
		packet.UID		= pPl->m_UID;
		packet.IsDead	= pPl->m_IsDead;
		packet.Deaths	= pPl->m_Deaths;

		if (pPlayerKiller)
			packet.KillerUID = pPlayerKiller->GetUID();

		ServerHelper::SendBroadcast(packet);

		PlayerScoreUpdatedEvent playerScoreUpdatedEvent(pPlayer);
		EventQueue::SendEventImmediate(playerScoreUpdatedEvent);

		PlayerAliveUpdatedEvent playerAliveUpdatedEvent(pPlayer, pPlayerKiller);
		EventQueue::SendEventImmediate(playerAliveUpdatedEvent);

		if (pPlayerKiller)
			SetPlayerKills(pPlayerKiller, pPlayerKiller->GetKills() + 1);
	}
}

void PlayerManagerServer::SetPlayerReady(const Player* pPlayer, bool ready)
{
	if (pPlayer->m_IsReady != ready)
	{
		Player* pPl = const_cast<Player*>(pPlayer);
		pPl->m_IsReady = ready;

		PacketPlayerReady packet;
		packet.UID		= pPl->m_UID;
		packet.IsReady	= pPl->m_IsReady;
		ServerHelper::SendBroadcast(packet);

		PlayerReadyUpdatedEvent event(pPlayer);
		EventQueue::SendEventImmediate(event);
	}
}

void PlayerManagerServer::SetPlayerHost(const Player* pPlayer)
{
	if (!pPlayer->m_IsHost)
	{
		Player* pOldHost = nullptr;
		for (auto& pair : s_Players)
		{
			Player& player = pair.second;
			if (player.IsHost())
			{
				player.m_IsHost = false;
				pOldHost = &player;
			}
		}

		Player* pPl = const_cast<Player*>(pPlayer);
		pPl->m_IsHost = true;

		PacketPlayerHost packet;
		packet.UID = pPl->m_UID;
		ServerHelper::SendBroadcast(packet);

		if (pOldHost)
		{
			PlayerHostUpdatedEvent event(pOldHost);
			EventQueue::SendEventImmediate(event);
		}

		PlayerHostUpdatedEvent event(pPlayer);
		EventQueue::SendEventImmediate(event);
	}
}

void PlayerManagerServer::SetPlayerTeam(const Player* pPlayer, uint8 team)
{
	if (pPlayer->m_Team != team)
	{
		Player* pPl = const_cast<Player*>(pPlayer);
		pPl->m_Team = team;

		PacketPlayerScore packet;
		FillPacketPlayerScore(&packet, pPl);
		ServerHelper::SendBroadcast(packet);

		PlayerScoreUpdatedEvent event(pPlayer);
		EventQueue::SendEventImmediate(event);
	}
}

void PlayerManagerServer::SetPlayerKills(const Player* pPlayer, uint8 kills)
{
	if (pPlayer->m_Kills != kills)
	{
		Player* pPl = const_cast<Player*>(pPlayer);
		pPl->m_Kills = kills;

		PacketPlayerScore packet;
		FillPacketPlayerScore(&packet, pPl);
		ServerHelper::SendBroadcast(packet);

		PlayerScoreUpdatedEvent event(pPlayer);
		EventQueue::SendEventImmediate(event);
	}
}

void PlayerManagerServer::SetPlayerDeaths(const Player* pPlayer, uint8 deaths)
{
	if (pPlayer->m_Deaths != deaths)
	{
		Player* pPl = const_cast<Player*>(pPlayer);
		pPl->m_Deaths = deaths;

		PacketPlayerScore packet;
		FillPacketPlayerScore(&packet, pPl);
		ServerHelper::SendBroadcast(packet);

		PlayerScoreUpdatedEvent event(pPlayer);
		EventQueue::SendEventImmediate(event);
	}
}

void PlayerManagerServer::SetPlayerFlagsCaptured(const Player* pPlayer, uint8 flagsCaptured)
{
	if (pPlayer->m_FlagsCaptured != flagsCaptured)
	{
		Player* pPl = const_cast<Player*>(pPlayer);
		pPl->m_FlagsCaptured = flagsCaptured;

		PacketPlayerScore packet;
		FillPacketPlayerScore(&packet, pPl);
		ServerHelper::SendBroadcast(packet);

		PlayerScoreUpdatedEvent event(pPlayer);
		EventQueue::SendEventImmediate(event);
	}
}

void PlayerManagerServer::SetPlayerFlagsDefended(const Player* pPlayer, uint8 flagsDefended)
{
	if (pPlayer->m_FlagsDefended != flagsDefended)
	{
		Player* pPl = const_cast<Player*>(pPlayer);
		pPl->m_FlagsDefended = flagsDefended;

		PacketPlayerScore packet;
		FillPacketPlayerScore(&packet, pPl);
		ServerHelper::SendBroadcast(packet);

		PlayerScoreUpdatedEvent event(pPlayer);
		EventQueue::SendEventImmediate(event);
	}
}

void PlayerManagerServer::SetPlayerStats(const Player* pPlayer, uint8 team, uint8 kills, uint8 deaths, uint8 flagsCaptured, uint8 flagsDefended)
{
	Player* pPl = const_cast<Player*>(pPlayer);
	bool changed = false;

	if (pPlayer->m_Team != team)
	{
		pPl->m_Team = team;
		changed = true;
	}
	if (pPlayer->m_Kills != kills)
	{
		pPl->m_Kills = kills;
		changed = true;
	}
	if (pPlayer->m_Deaths != deaths)
	{
		pPl->m_Deaths = deaths;
		changed = true;
	}
	if (pPlayer->m_FlagsCaptured != flagsCaptured)
	{
		pPl->m_FlagsCaptured = flagsCaptured;
		changed = true;
	}
	if (pPlayer->m_FlagsDefended != flagsDefended)
	{
		pPl->m_FlagsDefended = flagsDefended;
		changed = true;
	}

	if (changed)
	{
		PacketPlayerScore packet;
		FillPacketPlayerScore(&packet, pPl);
		ServerHelper::SendBroadcast(packet);

		PlayerScoreUpdatedEvent event(pPlayer);
		EventQueue::SendEventImmediate(event);
	}
}

void PlayerManagerServer::KickPlayers(uint8 count)
{
	for (auto& pair : s_Players)
	{
		const Player* pPlayer = &pair.second;
		if (!pPlayer->IsHost())
		{
			ServerHelper::DisconnectPlayer(pPlayer, "Kicked");
			if (--count == 0)
				return;
		}
	}
}

void PlayerManagerServer::FillPacketPlayerScore(PacketPlayerScore* pPacket, const Player* pPlayer)
{
	pPacket->UID			= pPlayer->m_UID;
	pPacket->Team			= pPlayer->m_Team;
	pPacket->Kills			= pPlayer->m_Kills;
	pPacket->Deaths			= pPlayer->m_Deaths;
	pPacket->FlagsCaptured	= pPlayer->m_FlagsCaptured;
	pPacket->FlagsDefended	= pPlayer->m_FlagsDefended;
}

void PlayerManagerServer::AutoSelectTeam(Player* pPlayer)
{
	TArray<const Player*> pPlayersTeam1;
	TArray<const Player*> pPlayersTeam2;
	GetPlayersOfTeam(pPlayersTeam1, 1);
	GetPlayersOfTeam(pPlayersTeam2, 2);
	pPlayer->m_Team = pPlayersTeam1.GetSize() > pPlayersTeam2.GetSize() ? 2 : 1;
}