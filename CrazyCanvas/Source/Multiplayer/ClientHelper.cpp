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
