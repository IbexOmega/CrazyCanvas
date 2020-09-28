#include "Game/ECS/Systems/Networking/ClientRemoteSystem.h"
#include "Game/ECS/Systems/Player/PlayerMovementSystem.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Player/ControllableComponent.h"

#include "ECS/ECSCore.h"

#include "Engine/EngineLoop.h"

namespace LambdaEngine
{
	glm::vec3 ClientRemoteSystem::s_StartPositions[10] =
	{
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 1.0f),
		glm::vec3(0.0f, 1.0f, 2.0f),
		glm::vec3(1.0f, 1.0f, 0.0f),
		glm::vec3(1.0f, 1.0f, 1.0f),
		glm::vec3(1.0f, 1.0f, 2.0f),
		glm::vec3(2.0f, 1.0f, 0.0f),
		glm::vec3(2.0f, 1.0f, 1.0f),
		glm::vec3(2.0f, 1.0f, 2.0f),
		glm::vec3(3.0f, 1.0f, 0.0f)
	};

	glm::vec3 ClientRemoteSystem::s_StartColors[10] =
	{
		glm::vec3(1.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f),
		glm::vec3(1.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 1.0f),
		glm::vec3(1.0f, 0.0f, 1.0f),
		glm::vec3(1.0f, 1.0f, 1.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(1.0f, 0.5f, 0.5f),
		glm::vec3(0.5f, 0.5f, 1.0f)
	};

	ClientRemoteSystem::ClientRemoteSystem() : 
		m_NetworkEntities(),
		m_Buffer(),
		m_pClient(nullptr),
		m_Entity(0),
		m_LastProcessedSimulationTick(-1),
		m_CurrentGameState(),
		m_Color()
	{
		RegisterSystem({});
	}

	ClientRemoteSystem::~ClientRemoteSystem()
	{

	}

	void ClientRemoteSystem::Tick(Timestamp deltaTime)
	{
		
	}

	void ClientRemoteSystem::TickMainThread(Timestamp deltaTime)
	{
		if (m_LastProcessedSimulationTick < m_CurrentGameState.SimulationTick)
		{
			m_LastProcessedSimulationTick = m_CurrentGameState.SimulationTick;
			//networkObject.frame = m_CurrentGameState.SimulationTick;
		}
		/*networkObject.position = _rigidBody.position;
		networkObject.rotation = _rigidBody.rotation;*/
	}

	void ClientRemoteSystem::FixedTickMainThread(Timestamp deltaTime)
	{
		if (m_pClient->IsConnected())
		{
			// Reset the current input - we don't want to re-use it if there no inputs in the queue
			//m_CurrentGameState = nullptr;

			// Process all available inputs each frame
			for (const GameState& gameState : m_Buffer)
			{
				m_CurrentGameState = gameState;
				PlayerUpdate(m_CurrentGameState);
			}
			m_Buffer.clear();




			auto* pPositionComponents = ECSCore::GetInstance()->GetComponentArray<PositionComponent>();

			NetworkSegment* pPacket = m_pClient->GetFreePacket(NetworkSegment::TYPE_PLAYER_ACTION);
			BinaryEncoder encoder(pPacket);
			encoder.WriteInt32(m_Entity);
			encoder.WriteInt32(m_LastProcessedSimulationTick);
			encoder.WriteVec3(pPositionComponents->GetData(m_Entity).Position);
			m_pClient->SendReliableBroadcast(pPacket);
		}
	}

	void ClientRemoteSystem::PlayerUpdate(const GameState& gameState)
	{
		// Set the velocity to zero, move the player based on the next input, then detect & resolve collisions
		//_rigidBody.velocity = Vector2.zero;
		PlayerMovementSystem::GetInstance().Move(m_Entity, EngineLoop::GetFixedTimestep(), gameState.DeltaForward, gameState.DeltaLeft);
		//PhysicsCollisions();
	}

	void ClientRemoteSystem::OnConnecting(IClient* pClient)
	{
		m_pClient = (ClientRemoteBase*)pClient;
	}

	void ClientRemoteSystem::OnConnected(IClient* pClient)
	{
		ECSCore* pECS = ECSCore::GetInstance();

		uint8 index = m_pClient->GetServer()->GetClientCount() % 10;
		const glm::vec3& position = s_StartPositions[index];
		m_Color = s_StartColors[index];

		m_Entity = pECS->CreateEntity();
		pECS->AddComponent<PositionComponent>(m_Entity,		{ position,	true });
		pECS->AddComponent<RotationComponent>(m_Entity,		{ glm::identity<glm::quat>(),	true });
		pECS->AddComponent<ScaleComponent>(m_Entity,		{ glm::vec3(1.0f),				true });
		pECS->AddComponent<NetworkComponent>(m_Entity,		{ (int32)m_Entity });
		pECS->AddComponent<ControllableComponent>(m_Entity, { false });

		NetworkSegment* pPacket = pClient->GetFreePacket(NetworkSegment::TYPE_ENTITY_CREATE);
		BinaryEncoder encoder = BinaryEncoder(pPacket);
		encoder.WriteBool(true);
		encoder.WriteInt32((int32)m_Entity);
		encoder.WriteVec3(position);
		encoder.WriteVec3(m_Color);
		pClient->SendReliable(pPacket, this);



		auto* pPositionComponents = pECS->GetComponentArray<PositionComponent>();
		const ClientMap& clients = m_pClient->GetClients();

		for (auto& clientPair : clients)
		{
			if (clientPair.second != m_pClient)
			{
				//Send to everyone already connected
				NetworkSegment* pPacket2 = clientPair.second->GetFreePacket(NetworkSegment::TYPE_ENTITY_CREATE);
				BinaryEncoder encoder2(pPacket2);
				encoder2.WriteBool(false);
				encoder2.WriteVec3(position);
				encoder2.WriteVec3(m_Color);
				clientPair.second->SendReliable(pPacket2, this);

				//Send everyone to my self
				ClientRemoteSystem* pHandler = (ClientRemoteSystem*)clientPair.second->GetHandler();
				PositionComponent& positionComponent = pPositionComponents->GetData(pHandler->m_Entity);

				NetworkSegment* pPacket3 = pClient->GetFreePacket(NetworkSegment::TYPE_ENTITY_CREATE);
				BinaryEncoder encoder3(pPacket3);
				encoder3.WriteBool(false);
				encoder3.WriteVec3(positionComponent.Position);
				encoder3.WriteVec3(pHandler->m_Color);
				pClient->SendReliable(pPacket3, this);
			}
		}
	}

	void ClientRemoteSystem::OnDisconnecting(IClient* pClient)
	{

	}

	void ClientRemoteSystem::OnDisconnected(IClient* pClient)
	{

	}

	void ClientRemoteSystem::OnPacketReceived(IClient* pClient, NetworkSegment* pPacket)
	{
		if (pPacket->GetType() == NetworkSegment::TYPE_PLAYER_ACTION)
		{
			GameState gameState = {};

			BinaryDecoder decoder(pPacket);
			decoder.ReadInt32(gameState.SimulationTick);
			decoder.ReadInt8(gameState.DeltaForward);
			decoder.ReadInt8(gameState.DeltaLeft);

			m_Buffer.insert(gameState); //Add Lock maybe, or change OnPacketReceived() to be runned by the main thread
		}
	}

	void ClientRemoteSystem::OnClientReleased(IClient* pClient)
	{
		delete this;
	}

	void ClientRemoteSystem::OnPacketDelivered(NetworkSegment* pPacket)
	{

	}

	void ClientRemoteSystem::OnPacketResent(NetworkSegment* pPacket, uint8 retries)
	{

	}

	void ClientRemoteSystem::OnPacketMaxTriesReached(NetworkSegment* pPacket, uint8 retries)
	{
		m_pClient->Disconnect("ClientRemoteSystem::OnPacketMaxTriesReached()");
	}
}