#include "Multiplayer/ClientHelper.h"

#include "Networking/API/UDP/NetworkDiscovery.h"

using namespace LambdaEngine;

void ClientHelper::AddNetworkDiscoveryTarget(LambdaEngine::IPAddress* pAddress)
{
	NetworkDiscovery::AddTarget(pAddress);
}

void ClientHelper::RemoveNetworkDiscoveryTarget(LambdaEngine::IPAddress* pAddress)
{
	NetworkDiscovery::RemoveTarget(pAddress);
}

void ClientHelper::Disconnect(const LambdaEngine::String& reason)
{
	ClientSystem::GetInstance().GetClient()->Disconnect(reason);
}
