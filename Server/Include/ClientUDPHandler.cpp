#include "ClientUDPHandler.h"

#include "Networking/API/IClientUDP.h"
#include "Networking/API/NetworkPacket.h"

#include "Log/Log.h"

ClientUDPHandler::ClientUDPHandler()
{
}

ClientUDPHandler::~ClientUDPHandler()
{
}

void ClientUDPHandler::OnConnectingUDP(LambdaEngine::IClientUDP* pClient)
{
	LOG_MESSAGE("OnConnectingUDP()");
}

void ClientUDPHandler::OnConnectedUDP(LambdaEngine::IClientUDP* pClient)
{
	LOG_MESSAGE("OnConnectedUDP()");
}

void ClientUDPHandler::OnDisconnectingUDP(LambdaEngine::IClientUDP* pClient)
{
	LOG_MESSAGE("OnDisconnectingUDP()");
}

void ClientUDPHandler::OnDisconnectedUDP(LambdaEngine::IClientUDP* pClient)
{
	LOG_MESSAGE("OnDisconnectedUDP()");
	pClient->Release();
	delete this;
}

void ClientUDPHandler::OnPacketReceivedUDP(LambdaEngine::IClientUDP* pClient, LambdaEngine::NetworkPacket* pPacket)
{
	LOG_MESSAGE("OnPacketReceivedUDP()");
}
