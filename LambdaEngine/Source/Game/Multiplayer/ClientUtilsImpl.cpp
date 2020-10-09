#include "Game/Multiplayer/ClientUtilsImpl.h"

#include "Networking/API/NetworkSegment.h"

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
		ASSERT(pair != m_NetworkUIDToEntityMapper.end());
		return pair->second;
	}

	void ClientUtilsImpl::RegisterEntity(Entity entity, int32 networkUID)
	{
		auto pair = m_NetworkUIDToEntityMapper.find(networkUID);
		ASSERT(pair == m_NetworkUIDToEntityMapper.end());
		m_NetworkUIDToEntityMapper.insert({ networkUID, entity });
	}
}