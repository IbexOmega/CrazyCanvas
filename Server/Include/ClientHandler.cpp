#include "ClientHandler.h"

#include "Networking/API/IClient.h"
#include "Networking/API/NetworkSegment.h"
#include "Networking/API/BinaryDecoder.h"
#include "Networking/API/BinaryEncoder.h"

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

	/*LambdaEngine::NetworkSegment* pPacket = pClient->GetFreePacket(69);
	LambdaEngine::BinaryEncoder encoder(pPacket);
	encoder.WriteString("Christoffer");
	pClient->SendReliable(pPacket, this);*/
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
	if (counter + 1 != decoder.ReadUInt32())
	{
		//DEBUGBREAK();
	}
	counter++;
	LOG_MESSAGE("%d", (int)counter);
}

void ClientHandler::OnClientReleased(LambdaEngine::IClient* pClient)
{
	UNREFERENCED_VARIABLE(pClient);
	delete this;
}

void ClientHandler::OnPacketDelivered(LambdaEngine::NetworkSegment* pPacket)
{
	LOG_ERROR("OnPacketDelivered(%s)", pPacket->ToString().c_str());
}

void ClientHandler::OnPacketResent(LambdaEngine::NetworkSegment* pPacket, uint8 retries)
{

}

void ClientHandler::OnPacketMaxTriesReached(LambdaEngine::NetworkSegment* pPacket, uint8 retries)
{

}
