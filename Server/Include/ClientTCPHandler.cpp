#include "ClientTCPHandler.h"

#include "Network/API/NetworkPacket.h"
#include "Network/API/ClientTCP2.h"

#include "Log/Log.h"

ClientTCPHandler::ClientTCPHandler()
{

}

ClientTCPHandler::~ClientTCPHandler()
{
}

void ClientTCPHandler::OnClientConnected(LambdaEngine::ClientTCP2* client)
{
	LOG_MESSAGE("OnClientConnected");
}

void ClientTCPHandler::OnClientDisconnected(LambdaEngine::ClientTCP2* client)
{
	LOG_MESSAGE("OnClientDisconnected");
}

void ClientTCPHandler::OnClientFailedConnecting(LambdaEngine::ClientTCP2* client)
{
	LOG_MESSAGE("OnClientFailedConnecting");
}

void ClientTCPHandler::OnClientPacketReceived(LambdaEngine::ClientTCP2* client, LambdaEngine::NetworkPacket* packet)
{
	if (packet->ReadPacketType() == LambdaEngine::PACKET_TYPE_USER_DATA)
	{
		std::string str;
		packet->ReadString(str);
		LOG_MESSAGE(str.c_str());
	}
}
