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

		NetworkPacket* reponse = DBG_NEW NetworkPacket(PACKET_TYPE_USER_DATA);
		reponse->WriteString("Nej jag klar!");
		client->SendPacket(reponse);
	}
}

void ClientUDPHandler::OnClientErrorUDP(LambdaEngine::IClientUDP* client)
{
	LOG_MESSAGE("OnClientErrorUDP");
}

void ClientUDPHandler::OnClientStoppedUDP(LambdaEngine::IClientUDP* client)
{
	LOG_MESSAGE("OnClientStoppedUDP");
}
