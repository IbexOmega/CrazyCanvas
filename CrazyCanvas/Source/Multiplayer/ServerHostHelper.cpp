#include "Multiplayer/ServerHostHelper.h"

#include "Math/Random.h"

using namespace LambdaEngine;

int32 ServerHostHelper::s_ClientHostID = -1;
int32 ServerHostHelper::s_AuthenticationID = -1;
bool ServerHostHelper::s_IsHost = false;

void ServerHostHelper::SetClientHostID(int32 serverHostID)
{
	s_ClientHostID = serverHostID;
}

void ServerHostHelper::SetAuthenticationID(int32 clientHostID)
{
	s_AuthenticationID = clientHostID;
}

void ServerHostHelper::SetIsHost(bool host)
{
	s_IsHost = host;
}

int32 ServerHostHelper::GetClientHostID()
{
	return s_ClientHostID;
}

int32 ServerHostHelper::GetAuthenticationHostID()
{
	return s_AuthenticationID;
}

bool ServerHostHelper::IsHost()
{
	return s_IsHost;
}

void ServerHostHelper::Init()
{
	s_ClientHostID = Random::Int32();
	s_AuthenticationID = Random::Int32();
}
