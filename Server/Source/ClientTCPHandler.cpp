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

void ClientTCPHandler::OnClientConnectedTCP(LambdaEngine::ClientTCP* client)
{
	LOG_MESSAGE("OnClientConnected");
}

void ClientTCPHandler::OnClientDisconnectedTCP(LambdaEngine::ClientTCP* client)
{
	LOG_MESSAGE("OnClientDisconnected");
}

void ClientTCPHandler::OnClientFailedConnectingTCP(LambdaEngine::ClientTCP* client)
{
	LOG_MESSAGE("OnClientFailedConnecting");
}

void ClientTCPHandler::OnClientPacketReceivedTCP(LambdaEngine::ClientTCP* client, LambdaEngine::NetworkPacket* packet)
{
	if (packet->ReadPacketType() == LambdaEngine::PACKET_TYPE_USER_DATA)
	{
		std::string str;
		packet->ReadString(str);
		LOG_MESSAGE(str.c_str());
	}
}
