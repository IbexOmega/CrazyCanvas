#include "Lobby/PlayerManagerServer.h"

#include "Game/Multiplayer/MultiplayerUtils.h"

#include "Multiplayer/ServerHelper.h"

#include "Multiplayer/Packet/PacketPlayerDied.h"
#include "Multiplayer/Packet/PacketPlayerHost.h"
#include "Multiplayer/Packet/PacketPlayerPing.h"

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
				pPlayer->m_Ping = (uint16)pClient->GetStatistics()->GetPing().AsMilliSeconds();
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

	if (s_Players.size() == 1)
	{
		SetPlayerHost(pPlayer);
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
	HandlePlayerLeft(pClient->GetUID());

	PacketLeave packet;
	packet.UID = pClient->GetUID();
	ServerHelper::SendBroadcast(packet, nullptr, pClient);
}

bool PlayerManagerServer::HasPlayerAuthority(const IClient* pClient)
{
	const Player* pPlayer = GetPlayer(pClient);
	return pPlayer != nullptr && pPlayer->IsHost();
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

void PlayerManagerServer::FillPacketPlayerScore(PacketPlayerScore* pPacket, const Player* pPlayer)
{
	pPacket->UID			= pPlayer->m_UID;
	pPacket->Team			= pPlayer->m_Team;
	pPacket->Kills			= pPlayer->m_Kills;
	pPacket->Deaths			= pPlayer->m_Deaths;
	pPacket->FlagsCaptured	= pPlayer->m_FlagsCaptured;
	pPacket->FlagsDefended	= pPlayer->m_FlagsDefended;
}
