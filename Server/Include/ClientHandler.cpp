#include "ClientHandler.h"

#include "Networking/API/IClient.h"
#include "Networking/API/NetworkSegment.h"
#include "Networking/API/BinaryDecoder.h"
#include "Networking/API/BinaryEncoder.h"
#include "Engine/EngineLoop.h"

#include "Log/Log.h"

ClientHandler::ClientHandler() :
	m_Counter(0)
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

	using namespace LambdaEngine;

	//LOG_MESSAGE("OnPacketReceived()");

	LambdaEngine::BinaryDecoder decoder(pPacket);
	
	uint32 recievedNumber = decoder.ReadUInt32();

	if (m_Counter + 1 == recievedNumber)
	{
		m_Counter = recievedNumber;
		//LOG_MESSAGE("%d", recievedNumber);
	}
	else
	{
		DEBUGBREAK();
	}

	if (recievedNumber == 1)
	{
		LOG_MESSAGE("Benchmark Timer Started.");
		m_BenchMarkTimer = EngineLoop::GetTimeSinceStart();
	}
	else if (recievedNumber == 100000)
	{
		m_BenchMarkTimer = EngineLoop::GetTimeSinceStart() - m_BenchMarkTimer;
		LOG_MESSAGE("Benchmark Timer Ended.");
		LOG_MESSAGE("Bytes Received during benchmark: %d", pClient->GetStatistics()->GetBytesReceived());
		LOG_MESSAGE("Benchmark Timer: %f", m_BenchMarkTimer.AsSeconds());
	}

	//LOG_MESSAGE("%d", (int)m_Counter);
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
	UNREFERENCED_VARIABLE(pPacket);
	UNREFERENCED_VARIABLE(retries);
}

void ClientHandler::OnPacketMaxTriesReached(LambdaEngine::NetworkSegment* pPacket, uint8 retries)
{
	UNREFERENCED_VARIABLE(pPacket);
	UNREFERENCED_VARIABLE(retries);
}
