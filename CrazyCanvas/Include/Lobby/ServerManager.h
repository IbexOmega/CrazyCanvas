#pragma once

#include "LambdaEngine.h"

#include "Containers/THashTable.h"

#include "Time/API/Timestamp.h"

#include "Application/API/Events/NetworkEvents.h"

#include "Lobby/ServerInfo.h"

class ServerManager
{
	friend class CrazyCanvas;

public:
	DECL_STATIC_CLASS(ServerManager);

	static const LambdaEngine::THashTable<uint64, ServerInfo>& GetServersLAN();
	static const LambdaEngine::THashTable<uint64, ServerInfo>& GetServersWAN();
	static void RegisterNewServer(const ServerInfo& serverInfo);

private:
	static void Init();
	static void Release();
	static void Tick();
	static bool OnServerResponse(const LambdaEngine::ServerDiscoveredEvent& event);
	static void AddOrUpdateServer(LambdaEngine::THashTable<uint64, ServerInfo>& table, const ServerInfo& serverInfo);
	static void RemoveOldServers(LambdaEngine::THashTable<uint64, ServerInfo>& table, LambdaEngine::Timestamp currentTime);

private:
	static inline LambdaEngine::THashTable<uint64, ServerInfo> s_ServersLAN;
	static inline LambdaEngine::THashTable<uint64, ServerInfo> s_ServersWAN;
	static inline LambdaEngine::Timestamp s_TimestampLastCheck;
	static inline LambdaEngine::Timestamp s_Interval		= LambdaEngine::Timestamp::Seconds(1);
	static inline LambdaEngine::Timestamp s_MaxOfflineTime	= LambdaEngine::Timestamp::Seconds(5);
};