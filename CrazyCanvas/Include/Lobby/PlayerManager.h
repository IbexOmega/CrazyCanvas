#pragma once

#include "Containers/String.h"
#include "Containers/THashTable.h"

#include "Multiplayer/Packet/MultiplayerEvents.h"
#include "Multiplayer/Packet/PacketJoin.h"
#include "Multiplayer/Packet/PacketLeave.h"

#include "Lobby/Player.h"

#include "Networking/API/IClient.h"

#include "Application/API/Events/NetworkEvents.h"

class PlayerManager
{
	friend class CrazyCanvas;
public:
	DECL_STATIC_CLASS(PlayerManager);

	static const Player* GetPlayer(uint64 uid);
	static const Player* GetPlayer(LambdaEngine::IClient* pClient);
	static const Player* GetPlayerLocal();

	static void RegisterLocalPlayer(const LambdaEngine::String& name, LambdaEngine::IClient* pClient);

private:
	static void Init();
	static void Release();

	static bool OnPacketJoinReceived(const PacketReceivedEvent<PacketJoin>& event);
	static bool OnPacketLeaveReceived(const PacketReceivedEvent<PacketLeave>& event);
	static bool OnClientDisconnected(const LambdaEngine::ClientDisconnectedEvent& event);

	static void HandlePlayerJoined(LambdaEngine::IClient* pClient, const Player& player);
	static void HandlePlayerLeft(LambdaEngine::IClient* pClientToExclude, uint64 uid);

private:
	static LambdaEngine::THashTable<uint64, Player> m_Players;
};