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
		return (Entity)networkUID;
	}

	int32 ServerUtilsImpl::GetNetworkUID(Entity entity) const
	{
		return (int32)entity;
	}

	void ServerUtilsImpl::RegisterEntity(Entity entity, int32 networkUID)
	{
		UNREFERENCED_VARIABLE(entity);
		UNREFERENCED_VARIABLE(networkUID);
	}

	void ServerUtilsImpl::UnregisterEntity(Entity entity)
	{
		UNREFERENCED_VARIABLE(entity);
	}
}