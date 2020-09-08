#include "ClientUDPHandler.h"

#include "Networking/API/IClientUDP.h"
#include "Networking/API/NetworkPacket.h"
#include "Networking/API/BinaryDecoder.h"

#include "Log/Log.h"

ClientUDPHandler::ClientUDPHandler()
{
}

ClientUDPHandler::~ClientUDPHandler()
{
}

void ClientUDPHandler::OnConnectingUDP(LambdaEngine::IClientUDP* pClient)
{
	UNREFERENCED_VARIABLE(pClient);
	LOG_MESSAGE("OnConnectingUDP()");
}

void ClientUDPHandler::OnConnectedUDP(LambdaEngine::IClientUDP* pClient)
{
	UNREFERENCED_VARIABLE(pClient);
	LOG_MESSAGE("OnConnectedUDP()");
}

void ClientUDPHandler::OnDisconnectingUDP(LambdaEngine::IClientUDP* pClient)
{
	UNREFERENCED_VARIABLE(pClient);
	LOG_MESSAGE("OnDisconnectingUDP()");
}

void ClientUDPHandler::OnDisconnectedUDP(LambdaEngine::IClientUDP* pClient)
{
	UNREFERENCED_VARIABLE(pClient);
	LOG_MESSAGE("OnDisconnectedUDP()");
	pClient->Release();
	delete this;
}

void ClientUDPHandler::OnPacketReceivedUDP(LambdaEngine::IClientUDP* pClient, LambdaEngine::NetworkSegment* pPacket)
{
	UNREFERENCED_VARIABLE(pClient);
	UNREFERENCED_VARIABLE(pPacket);

	using namespace LambdaEngine;

	LOG_MESSAGE("OnPacketReceivedUDP()");

	LambdaEngine::BinaryDecoder decoder(pPacket);
	std::string name = decoder.ReadString();
	LOG_MESSAGE(name.c_str());
}
