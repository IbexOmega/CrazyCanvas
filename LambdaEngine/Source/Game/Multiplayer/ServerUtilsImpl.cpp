#include "Game/Multiplayer/ServerUtilsImpl.h"

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
}