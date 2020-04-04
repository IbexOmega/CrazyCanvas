#include "ClientTCPHandler.h"

#include "Network/API/NetworkPacket.h"
#include "Network/API/ClientTCP.h"

#include "Log/Log.h"

ClientTCPHandler::ClientTCPHandler()
{

}

ClientTCPHandler::~ClientTCPHandler()
{
}

void ClientTCPHandler::OnClientConnected(LambdaEngine::ClientTCP* client)
{
}

void ClientTCPHandler::OnClientDisconnected(LambdaEngine::ClientTCP* client)
{
}

void ClientTCPHandler::OnClientFailedConnecting(LambdaEngine::ClientTCP* client)
{
}

void ClientTCPHandler::OnClientPacketReceived(LambdaEngine::ClientTCP* client, LambdaEngine::NetworkPacket* packet)
{
	if (packet->ReadPacketType() == LambdaEngine::PACKET_TYPE_USER_DATA)
	{
		std::string str;
		packet->ReadString(str);
		LOG_MESSAGE(str.c_str());
	}
}
