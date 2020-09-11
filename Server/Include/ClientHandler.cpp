#include "ClientHandler.h"

#include "Networking/API/IClient.h"
#include "Networking/API/NetworkSegment.h"
#include "Networking/API/BinaryDecoder.h"

#include "Log/Log.h"

ClientHandler::ClientHandler()
{

}

ClientHandler::~ClientHandler()
{

}

void ClientHandler::OnConnecting(LambdaEngine::IClient* pClient)
{
	UNREFERENCED_VARIABLE(pClient);
	LOG_MESSAGE("OnConnecting()");
}

void ClientHandler::OnConnected(LambdaEngine::IClient* pClient)
{
	UNREFERENCED_VARIABLE(pClient);
	LOG_MESSAGE("OnConnected()");
}

void ClientHandler::OnDisconnecting(LambdaEngine::IClient* pClient)
{
	UNREFERENCED_VARIABLE(pClient);
	LOG_MESSAGE("OnDisconnecting()");
}

void ClientHandler::OnDisconnected(LambdaEngine::IClient* pClient)
{
	UNREFERENCED_VARIABLE(pClient);
	LOG_MESSAGE("OnDisconnected()");
}

void ClientHandler::OnPacketReceived(LambdaEngine::IClient* pClient, LambdaEngine::NetworkSegment* pPacket)
{
	UNREFERENCED_VARIABLE(pClient);
	UNREFERENCED_VARIABLE(pPacket);

	using namespace LambdaEngine;

	LOG_MESSAGE("OnPacketReceived()");

	LambdaEngine::BinaryDecoder decoder(pPacket);
	std::string name = decoder.ReadString();
	LOG_MESSAGE(name.c_str());
}

void ClientHandler::OnClientReleased(LambdaEngine::IClient* pClient)
{
	UNREFERENCED_VARIABLE(pClient);
	delete this;
}
