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
		return networkUID;
	}

	void ServerUtilsImpl::RegisterEntity(Entity entity, int32 networkUID)
	{
		UNREFERENCED_VARIABLE(entity);
		UNREFERENCED_VARIABLE(networkUID);
	}
}