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
		ASSERT(pair != m_NetworkUIDToEntityMapper.end());
		return pair->second;
	}

	void ClientUtilsImpl::RegisterEntity(Entity entity, int32 networkUID)
	{
		auto pair = m_NetworkUIDToEntityMapper.find(networkUID);
		ASSERT(pair == m_NetworkUIDToEntityMapper.end());
		m_NetworkUIDToEntityMapper.insert({ networkUID, entity });
	}

	uint64 ClientUtilsImpl::GetSaltAsUID(IClient* pClient)
	{
		return pClient->GetStatistics()->GetRemoteSalt();
	}
}