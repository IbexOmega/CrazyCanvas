#include "ClientUDPHandler.h"

#include "Network/API/NetworkPacket.h"
#include "Network/API/UDP/IClientUDP.h"

#include "Log/Log.h"

ClientUDPHandler::ClientUDPHandler()
{
	
}

ClientUDPHandler::~ClientUDPHandler()
{
}

void ClientUDPHandler::OnClientPacketReceivedUDP(LambdaEngine::IClientUDP* client, LambdaEngine::NetworkPacket* packet)
{
	using namespace LambdaEngine;

	if (packet->ReadPacketType() == PACKET_TYPE_USER_DATA)
	{
		std::string str;
		packet->ReadString(str);
		LOG_MESSAGE(str.c_str());

		LambdaEngine::NetworkPacket* packetResponse = DBG_NEW LambdaEngine::NetworkPacket(LambdaEngine::PACKET_TYPE_USER_DATA);
		packetResponse->WriteString("Server Response UDP");
		client->SendPacket(packetResponse, true);
	}
}

void ClientUDPHandler::OnClientErrorUDP(LambdaEngine::IClientUDP* client)
{
	LOG_MESSAGE("OnClientErrorUDP()");
	UNREFERENCED_VARIABLE(client);
}

void ClientUDPHandler::OnClientStoppedUDP(LambdaEngine::IClientUDP* client)
{
	LOG_MESSAGE("OnClientStoppedUDP()");
	UNREFERENCED_VARIABLE(client);
}
