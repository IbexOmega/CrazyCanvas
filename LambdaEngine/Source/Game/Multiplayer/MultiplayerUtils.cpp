#include "Game/Multiplayer/MultiplayerUtils.h"

#include "Networking/API/IClient.h"
#include "Networking/API/NetworkStatistics.h"

namespace LambdaEngine
{
	bool MultiplayerUtils::s_IsServer = false;
	bool MultiplayerUtils::s_IsSinglePlayer = false;

	std::unordered_map<int32, Entity> MultiplayerUtils::s_NetworkUIDToEntityMapper;
	std::unordered_map<int32, Entity> MultiplayerUtils::s_EntityToNetworkUIDMapper;

	bool MultiplayerUtils::IsServer()
	{
		return s_IsServer;
	}

	Entity MultiplayerUtils::GetEntity(int32 networkUID)
	{
		auto pair = s_NetworkUIDToEntityMapper.find(networkUID);
		return pair == s_NetworkUIDToEntityMapper.end() ? UINT32_MAX : pair->second;
	}

	int32 MultiplayerUtils::GetNetworkUID(Entity entity)
	{
		auto pair = s_EntityToNetworkUIDMapper.find(entity);
		return pair == s_EntityToNetworkUIDMapper.end() ? UINT32_MAX : pair->second;
	}

	void MultiplayerUtils::RegisterEntity(Entity entity, int32 networkUID)
	{
		ASSERT(m_NetworkUIDToEntityMapper.find(networkUID) == s_NetworkUIDToEntityMapper.end());
		s_NetworkUIDToEntityMapper.insert({ networkUID, entity });
		s_EntityToNetworkUIDMapper.insert({ entity, networkUID });
	}

	void MultiplayerUtils::UnregisterEntity(Entity entity)
	{
		s_NetworkUIDToEntityMapper.erase(GetNetworkUID(entity));
		s_EntityToNetworkUIDMapper.erase(entity);
	}

	bool MultiplayerUtils::IsSingleplayer()
	{
		return s_IsSinglePlayer;
	}

	bool MultiplayerUtils::HasWriteAccessToEntity(Entity entity)
	{
		UNREFERENCED_VARIABLE(entity);

		//TODO: Add code checks depending on server or client and ownership of entity
		return true;
	}

	void MultiplayerUtils::SetIsSingleplayer(bool value)
	{
		s_IsSinglePlayer = value;
	}

	void MultiplayerUtils::Init(bool server)
	{
		s_IsServer = server;
	}

	void MultiplayerUtils::Reset()
	{
		s_NetworkUIDToEntityMapper.clear();
		s_EntityToNetworkUIDMapper.clear();
	}
}