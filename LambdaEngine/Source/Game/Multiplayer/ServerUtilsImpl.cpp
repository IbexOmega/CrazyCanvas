#include "Game/Multiplayer/ServerUtilsImpl.h"

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
		return networkUID;
	}

	void ServerUtilsImpl::RegisterEntity(Entity entity, int32 networkUID)
	{

	}

	uint64 ServerUtilsImpl::GetSaltAsUID(IClient* pClient)
	{
		return pClient->GetStatistics()->GetSalt();
	}
}