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

void ServerHelper::DisconnectPlayer(const Player* pPlayer, const LambdaEngine::String& reason)
{
	ClientRemoteBase* pClient = ServerSystem::GetInstance().GetServer()->GetClient(pPlayer->GetUID());
	if(pClient)
		pClient->Disconnect(reason);
}

/*ClientRemoteBase* ServerHelper::GetClient(uint64 uid)
{
	return ServerSystem::GetInstance().GetServer()->GetClient(uid);
}*/
