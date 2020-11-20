#pragma once

#include "Containers/String.h"
#include "Containers/THashTable.h"

#include "Time/API/Timestamp.h"

#include "Multiplayer/Packet/PacketJoin.h"
#include "Multiplayer/Packet/PacketLeave.h"
#include "Multiplayer/Packet/PacketPlayerScore.h"

#include "Lobby/Player.h"

#include "Networking/API/IClient.h"

#include "Application/API/Events/NetworkEvents.h"

#include "Events/PacketEvents.h"

#include "ECS/Entity.h"

class PlayerManagerBase
{
public:
	DECL_SINGLETON_CLASS(PlayerManagerBase);

	static uint8 GetPlayerCount();
	static const Player* GetPlayer(uint64 uid);
	static const Player* GetPlayer(const LambdaEngine::IClient* pClient);
	static const Player* GetPlayer(LambdaEngine::Entity entity);
	static const LambdaEngine::THashTable<uint64, Player>& GetPlayers();
	static void GetPlayersOfTeam(LambdaEngine::TArray<const Player*>& players, uint8 team);
	static void SetPlayerEntity(const Player* pPlayer, LambdaEngine::Entity entity);

protected:
	static void Init();
	static void Release();
	static void Reset();

	static Player* GetPlayerNoConst(uint64 uid);
	static Player* GetPlayerNoConst(LambdaEngine::IClient* pClient);
	static Player* GetPlayerNoConst(LambdaEngine::Entity entity);

	static Player* HandlePlayerJoined(uint64 uid, const PacketJoin& packet);
	static bool HandlePlayerLeft(uint64 uid);

protected:
	static LambdaEngine::THashTable<uint64, Player> s_Players;
	static LambdaEngine::THashTable<LambdaEngine::Entity, uint64> s_PlayerEntityToUID;
};