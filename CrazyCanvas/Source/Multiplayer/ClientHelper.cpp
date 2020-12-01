#include "Multiplayer/ClientHelper.h"

#include "Networking/API/UDP/NetworkDiscovery.h"

using namespace LambdaEngine;

void ClientHelper::AddNetworkDiscoveryTarget(IPAddress* pAddress)
{
	NetworkDiscovery::AddTarget(pAddress);
}

void ClientHelper::RemoveNetworkDiscoveryTarget(IPAddress* pAddress)
{
	NetworkDiscovery::RemoveTarget(pAddress);
}

void ClientHelper::Disconnect(const String& reason)
{
	ClientSystem::GetInstance().GetClient()->Disconnect(reason);
}

void ClientHelper::SetTimeout(Timestamp time)
{
	ClientSystem::GetInstance().GetClient()->SetTimeout(time);
}

void ClientHelper::ResetTimeout()
{
	ClientSystem::GetInstance().GetClient()->ResetTimeout();
}