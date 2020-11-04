#include "Multiplayer/ServerHelper.h"

using namespace LambdaEngine;

void ServerHelper::SetMaxClients(uint8 clients)
{
	ServerSystem::GetInstance().GetServer()->SetMaxClients(clients);
}

void ServerHelper::SetIgnoreNewClients(bool ignore)
{
	ServerSystem::GetInstance().GetServer()->SetAcceptingConnections(!ignore);
}

ClientRemoteBase* ServerHelper::GetClient(uint64 uid)
{
	return ServerSystem::GetInstance().GetServer()->GetClient(uid);
}
