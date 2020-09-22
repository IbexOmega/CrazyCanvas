#include "ClientHandler.h"

#include "Networking/API/PlatformNetworkUtils.h"
#include "Engine/EngineLoop.h"

#include "ECS/ECSCore.h"

#include "Game/ECS/Components/Networking/NetworkComponent.h"
#include "Game/ECS/Components/Physics/Transform.h"


#include "Log/Log.h"

#define TYPE_ADD_ENTITY 1

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

glm::vec3 position(0.0f, 1.0f, 0.0f);
int colorIndex = 0;

void ClientHandler::OnConnected(LambdaEngine::IClient* pClient)
{
	UNREFERENCED_VARIABLE(pClient);
	LOG_MESSAGE("OnConnected()");

	using namespace LambdaEngine;


	if (colorIndex == 0)
		m_Color = glm::vec3(1.0f, 0.0f, 0.0f);
	else if (colorIndex == 1)
		m_Color = glm::vec3(0.0f, 1.0f, 0.0f);
	else if (colorIndex == 2)
		m_Color = glm::vec3(0.0f, 0.0f, 1.0f);
	else if (colorIndex == 3)
		m_Color = glm::vec3(1.0f, 1.0f, 0.0f);
	else if (colorIndex == 4)
		m_Color = glm::vec3(0.0f, 1.0f, 1.0f);
	else if (colorIndex == 5)
		m_Color = glm::vec3(1.0f, 0.0f, 1.0f);

	colorIndex++;
	m_Position = position;
	position.x += 2.0f;




	ECSCore* pECS = ECSCore::GetInstance();

	m_Entity = pECS->CreateEntity();
	pECS->AddComponent<PositionComponent>(m_Entity,	{ m_Position,	true });
	pECS->AddComponent<RotationComponent>(m_Entity,	{ glm::identity<glm::quat>(),	true });
	pECS->AddComponent<ScaleComponent>(m_Entity,	{ glm::vec3(1.0f),				true });
	pECS->AddComponent<NetworkComponent>(m_Entity,	{ (int32)m_Entity });

	NetworkSegment* pPacket = pClient->GetFreePacket(TYPE_ADD_ENTITY);
	BinaryEncoder encoder = BinaryEncoder(pPacket);
	encoder.WriteBool(true);
	encoder.WriteVec3(m_Position);
	encoder.WriteVec3(m_Color);
	pClient->SendReliable(pPacket, this);

	ClientRemoteBase* pClientRemote = (ClientRemoteBase*)pClient;

	const ClientMap& clients = pClientRemote->GetClients();

	for (auto& clientPair : clients)
	{
		if (clientPair.second != pClientRemote)
		{
			pPacket = clientPair.second->GetFreePacket(TYPE_ADD_ENTITY);
			encoder = BinaryEncoder(pPacket);
			encoder.WriteBool(false);
			encoder.WriteVec3(m_Position);
			encoder.WriteVec3(m_Color);
			clientPair.second->SendReliable(pPacket, this);

			ClientHandler* pHandler = (ClientHandler*)clientPair.second->GetHandler();

			pPacket = pClient->GetFreePacket(TYPE_ADD_ENTITY);
			encoder = BinaryEncoder(pPacket);
			encoder.WriteBool(false);
			encoder.WriteVec3(pHandler->m_Position);
			encoder.WriteVec3(pHandler->m_Color);
			pClient->SendReliable(pPacket, this);
		}
	}
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
