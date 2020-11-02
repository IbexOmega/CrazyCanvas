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
