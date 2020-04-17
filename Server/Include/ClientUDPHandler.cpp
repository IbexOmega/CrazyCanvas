#include "ClientUDPHandler.h"

#include "Networking/API/IClientUDP.h"
#include "Networking/API/NetworkPacket.h"

ClientUDPHandler::ClientUDPHandler()
{
}

ClientUDPHandler::~ClientUDPHandler()
{
}

void ClientUDPHandler::OnPacketUDPReceived(LambdaEngine::IClientUDP* pClient, LambdaEngine::NetworkPacket* pPacket)
{

}
