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

void PlayerManagerBase::Reset()
{
	s_Players.clear();
	s_PlayerEntityToUID.clear();
}

const Player* PlayerManagerBase::GetPlayer(uint64 uid)
{
	auto pair = s_Players.find(uid);
	return pair == s_Players.end() ? nullptr : &pair->second;
}

const Player* PlayerManagerBase::GetPlayer(const IClient* pClient)
{
	return GetPlayer(pClient->GetUID());
}

const Player* PlayerManagerBase::GetPlayer(Entity entity)
{
	auto pair = s_PlayerEntityToUID.find(entity);
	return pair == s_PlayerEntityToUID.end() ? nullptr : GetPlayer(pair->second);
}

const THashTable<uint64, Player>& PlayerManagerBase::GetPlayers()
{
	return s_Players;
}

Player* PlayerManagerBase::GetPlayerNoConst(uint64 uid)
{
	auto pair = s_Players.find(uid);
	return pair == s_Players.end() ? nullptr : &pair->second;
}

Player* PlayerManagerBase::GetPlayerNoConst(IClient* pClient)
{
	return GetPlayerNoConst(pClient->GetUID());
}

Player* PlayerManagerBase::GetPlayerNoConst(Entity entity)
{
	auto pair = s_PlayerEntityToUID.find(entity);
	return pair == s_PlayerEntityToUID.end() ? nullptr : GetPlayerNoConst(pair->second);
}

void PlayerManagerBase::RegisterPlayerEntity(uint64 uid, Entity entity)
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

bool PlayerManagerBase::HandlePlayerLeft(uint64 uid)
{
	auto pair = s_Players.find(uid);
	if (pair != s_Players.end())
	{
		const Player& player = pair->second;

		LOG_INFO("Player [%s] left! [%llu]", player.GetName().c_str(), player.GetUID());

		PlayerLeftEvent event(&player);
		EventQueue::SendEventImmediate(event);

		bool wasHost = player.IsHost();
		s_Players.erase(uid);

		return wasHost;
	}

	return false;
}
