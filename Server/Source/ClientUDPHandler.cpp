#include "ClientUDPHandler.h"

#include "Network/API/NetworkPacket.h"
#include "Network/API/IClientUDP.h"

#include "Log/Log.h"

ClientUDPHandler::ClientUDPHandler()
{

}

ClientUDPHandler::~ClientUDPHandler()
{
}

void ClientUDPHandler::OnClientPacketReceivedUDP(LambdaEngine::IClientUDP* client, LambdaEngine::NetworkPacket* packet)
{
	if (packet->ReadPacketType() == LambdaEngine::PACKET_TYPE_USER_DATA)
	{
		std::string str;
		packet->ReadString(str);
		LOG_MESSAGE(str.c_str());
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
