#include "Game/Multiplayer/Server/ServerUtilsImpl.h"

#include "Networking/API/IClient.h"

namespace LambdaEngine
{
	ServerUtilsImpl::ServerUtilsImpl()
	{

	}

	ServerUtilsImpl::~ServerUtilsImpl()
	{

	}

	Entity ServerUtilsImpl::GetEntity(int32 networkUID) const
	{
		auto entityIt = m_Entities.find(Entity(networkUID));
		return entityIt == m_Entities.end() ? UINT32_MAX : *entityIt;
	}

	int32 ServerUtilsImpl::GetNetworkUID(Entity entity) const
	{
		auto entityIt = m_Entities.find(entity);
		return entityIt == m_Entities.end() ? UINT32_MAX : int32(*entityIt);
	}

	void ServerUtilsImpl::RegisterEntity(Entity entity, int32 networkUID)
	{
		UNREFERENCED_VARIABLE(networkUID);

#ifdef LAMBDA_DEBUG
		VALIDATE(m_Entities.insert(entity).second);
#else
		m_Entities.insert(entity);
#endif
	}

	void ServerUtilsImpl::UnregisterEntity(Entity entity)
	{
		m_Entities.erase(entity);
	}
}