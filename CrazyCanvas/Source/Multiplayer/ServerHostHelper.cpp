#include "Multiplayer/ServerHostHelper.h"

#include "Math/Random.h"

using namespace LambdaEngine;

int32 ServerHostHelper::s_ClientHostID = -1;
int32 ServerHostHelper::s_AuthenticationID = -1;

void ServerHostHelper::SetClientHostID(int32 serverHostID)
{
	s_ClientHostID = serverHostID;
}

void ServerHostHelper::SetAuthenticationID(int32 clientHostID)
{
	s_AuthenticationID = clientHostID;
}

int32 ServerHostHelper::GetClientHostID()
{
	return s_ClientHostID;
}

int32 ServerHostHelper::GetAuthenticationHostID()
{
	return s_AuthenticationID;
}

void ServerHostHelper::Init()
{
	s_ClientHostID = Random::Int32();
	s_AuthenticationID = Random::Int32();
}
