#include "Game/Multiplayer/Client/ClientUtilsImpl.h"

#include "Networking/API/NetworkSegment.h"
#include "Networking/API/NetworkStatistics.h"

#include "Networking/API/IClient.h"

namespace LambdaEngine
{
	ClientUtilsImpl::ClientUtilsImpl()
	{

	}

	ClientUtilsImpl::~ClientUtilsImpl()
	{

	}

	Entity ClientUtilsImpl::GetEntity(int32 networkUID) const
	{
		auto pair = m_NetworkUIDToEntityMapper.find(networkUID);
		return pair == m_NetworkUIDToEntityMapper.end() ? UINT32_MAX : pair->second;
	}

	int32 ClientUtilsImpl::GetNetworkUID(Entity entity) const
	{
		auto pair = m_EntityToNetworkUIDMapper.find(entity);
		return pair == m_EntityToNetworkUIDMapper.end() ? UINT32_MAX : pair->second;
	}

	void ClientUtilsImpl::RegisterEntity(Entity entity, int32 networkUID)
	{
		ASSERT(m_NetworkUIDToEntityMapper.find(networkUID) == m_NetworkUIDToEntityMapper.end());
		m_NetworkUIDToEntityMapper.insert({ networkUID, entity });
		m_EntityToNetworkUIDMapper.insert({ entity, networkUID });
	}
}
