#include "Lobby/PlayerManagerBase.h"

#include "Game/Multiplayer/MultiplayerUtils.h"

#include "Application/API/Events/EventQueue.h"

#include "Events/PlayerEvents.h"

using namespace LambdaEngine;

THashTable<uint64, Player> PlayerManagerBase::s_Players;
THashTable<Entity, uint64> PlayerManagerBase::s_PlayerEntityToUID;

void PlayerManagerBase::Init()
{
	
}

void PlayerManagerBase::Release()
{

}

const Player* PlayerManagerBase::GetPlayer(uint64 uid)
{
    auto pair = s_Players.find(uid);
    return pair == s_Players.end() ? nullptr : &pair->second;
}

const Player* PlayerManagerBase::GetPlayer(LambdaEngine::IClient* pClient)
{
    return GetPlayer(pClient->GetUID());
}

const Player* PlayerManagerBase::GetPlayer(LambdaEngine::Entity entity)
{
	auto pair = s_PlayerEntityToUID.find(entity);
	return pair == s_PlayerEntityToUID.end() ? nullptr : GetPlayer(pair->second);
}

Player* PlayerManagerBase::GetPlayerNoConst(uint64 uid)
{
	auto pair = s_Players.find(uid);
	return pair == s_Players.end() ? nullptr : &pair->second;
}

Player* PlayerManagerBase::GetPlayerNoConst(LambdaEngine::IClient* pClient)
{
	return GetPlayerNoConst(pClient->GetUID());
}

Player* PlayerManagerBase::GetPlayerNoConst(LambdaEngine::Entity entity)
{
	auto pair = s_PlayerEntityToUID.find(entity);
	return pair == s_PlayerEntityToUID.end() ? nullptr : GetPlayerNoConst(pair->second);
}

void PlayerManagerBase::RegisterPlayerEntity(uint64 uid, LambdaEngine::Entity entity)
{
	auto pair = s_Players.find(uid);
	if (pair != s_Players.end())
	{
		s_PlayerEntityToUID.insert({ entity, uid });
		pair->second.m_Entity = entity;
	}
	else
	{
		LOG_ERROR("Failed to register player entity with client UID: %llu", uid);
	}
}

Player* PlayerManagerBase::HandlePlayerJoined(uint64 uid, const PacketJoin& packet)
{
	auto pair = s_Players.find(uid);
	if (pair == s_Players.end())
	{
		Player player;
		player.m_UID = uid;
		player.m_Name = packet.Name;

		Player* pPlayer = &s_Players.insert({ player.GetUID(), player }).first->second;

		LOG_INFO("Player [%s] joined! [%llu]", player.GetName().c_str(), player.GetUID());

		PlayerJoinedEvent newEvent(pPlayer);
		EventQueue::SendEventImmediate(newEvent);

		return pPlayer;
	}
	return &pair->second;
}

void PlayerManagerBase::HandlePlayerLeft(uint64 uid)
{
	auto pair = s_Players.find(uid);
	if (pair != s_Players.end())
	{
		const Player& player = pair->second;

		LOG_INFO("Player [%s] left! [%llu]", player.GetName().c_str(), player.GetUID());

		PlayerLeftEvent event(&player);
		EventQueue::SendEventImmediate(event);

		s_Players.erase(uid);
	}
}

bool PlayerManagerBase::UpdatePlayerFromPacket(Player* pPlayer, const PacketPlayerInfo* pPacket)
{
	bool changed = false;
	bool scoreChanged = false;

	if (pPlayer->m_Ping != pPacket->Ping)
	{
		changed = true;
		pPlayer->m_Ping = pPacket->Ping;
		PlayerPingUpdatedEvent event(pPlayer);
		EventQueue::SendEventImmediate(event);
	}
	if (pPlayer->m_State != pPacket->State)
	{
		changed = true;
		pPlayer->m_State = pPacket->State;
		PlayerStateUpdatedEvent event(pPlayer);
		EventQueue::SendEventImmediate(event);
	}
	if (pPlayer->m_Team != pPacket->Team)
	{
		changed = true;
		pPlayer->m_Team = pPacket->Team;
		PlayerTeamUpdatedEvent event(pPlayer);
		EventQueue::SendEventImmediate(event);
	}
	if (pPlayer->m_Kills != pPacket->Kills)
	{
		changed = true;
		scoreChanged = true;
		pPlayer->m_Kills = pPacket->Kills;
	}
	if (pPlayer->m_Deaths != pPacket->Deaths)
	{
		changed = true;
		scoreChanged = true;
		pPlayer->m_Deaths = pPacket->Deaths;
	}
	if (pPlayer->m_FlagsCaptured != pPacket->FlagsCaptured)
	{
		changed = true;
		scoreChanged = true;
		pPlayer->m_FlagsCaptured = pPacket->FlagsCaptured;
	}
	if (pPlayer->m_FlagsDefended != pPacket->FlagsDefended)
	{
		changed = true;
		scoreChanged = true;
		pPlayer->m_FlagsDefended = pPacket->FlagsDefended;
	}

	if (scoreChanged)
	{
		PlayerScoreUpdatedEvent event(pPlayer);
		EventQueue::SendEventImmediate(event);
	}

	return changed;
}

void PlayerManagerBase::UpdatePacketFromPlayer(PacketPlayerInfo* pPacket, const Player* pPlayer)
{
	pPacket->UID			= pPlayer->m_UID;
	pPacket->Ping			= pPlayer->m_Ping;
	pPacket->State			= pPlayer->m_State;
	pPacket->Team			= pPlayer->m_Team;
	pPacket->Kills			= pPlayer->m_Kills;
	pPacket->Deaths			= pPlayer->m_Deaths;
	pPacket->FlagsCaptured	= pPlayer->m_FlagsCaptured;
	pPacket->FlagsDefended	= pPlayer->m_FlagsDefended;
}
