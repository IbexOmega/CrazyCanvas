#include "ClientTCPHandler.h"

#include "Network/API/NetworkPacket.h"
#include "Network/API/TCP/ClientTCP.h"

#include "Log/Log.h"

ClientTCPHandler::ClientTCPHandler()
{

}

ClientTCPHandler::~ClientTCPHandler()
{
}

void ClientTCPHandler::OnClientConnectedTCP(LambdaEngine::ClientTCP* client)
{
	LOG_MESSAGE("OnClientConnectedTCP()");
	UNREFERENCED_VARIABLE(client);
}

void ClientTCPHandler::OnClientDisconnectedTCP(LambdaEngine::ClientTCP* client)
{
	LOG_MESSAGE("OnClientDisconnectedTCP()");
	UNREFERENCED_VARIABLE(client);
}

void ClientTCPHandler::OnClientFailedConnectingTCP(LambdaEngine::ClientTCP* client)
{
	LOG_MESSAGE("OnClientFailedConnectingTCP()");
	UNREFERENCED_VARIABLE(client);
}

void ClientTCPHandler::OnClientPacketReceivedTCP(LambdaEngine::ClientTCP* client, LambdaEngine::NetworkPacket* packet)
{
	if (packet->ReadPacketType() == LambdaEngine::PACKET_TYPE_USER_DATA)
	{
		std::string str;
		packet->ReadString(str);
		LOG_MESSAGE(str.c_str());

		LambdaEngine::NetworkPacket* packetResponse = DBG_NEW LambdaEngine::NetworkPacket(LambdaEngine::PACKET_TYPE_USER_DATA);
		packetResponse->WriteString("Server Response TCP");
		client->SendPacket(packetResponse, true);
	}
}
