#include "Game/Multiplayer/Server/ClientRemoteSystem.h"
#include "Game/Multiplayer/Server/ServerSystem.h"
#include "Game/Multiplayer/CharacterControllerHelper.h"
#include "Game/World/Player/PlayerActionSystem.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Physics/Collision.h"
#include "Game/ECS/Components/Networking/NetworkPositionComponent.h"
#include "Game/ECS/Systems/Physics/PhysicsSystem.h"

#include "ECS/ECSCore.h"

#include "Engine/EngineLoop.h"

#include "Application/API/Events/EventQueue.h"
#include "Application/API/Events/NetworkEvents.h"

#include "Game/Multiplayer/MultiplayerUtils.h"

namespace LambdaEngine
{
	ClientRemoteSystem::ClientRemoteSystem() :
		m_Buffer(),
		m_pClient(nullptr),
		m_CurrentGameState()
	{
		
	}

	ClientRemoteSystem::~ClientRemoteSystem()
	{

	}

	void ClientRemoteSystem::TickMainThread(Timestamp deltaTime)
	{
		UNREFERENCED_VARIABLE(deltaTime);
	}

	void ClientRemoteSystem::FixedTickMainThread(Timestamp deltaTime)
	{
		if (m_pClient->IsConnected())
		{
			const float32 dt = (float32)deltaTime.AsSeconds();
			Entity entityPlayer = MultiplayerUtils::GetEntityPlayer(m_pClient);

			if (entityPlayer < UINT32_MAX)
			{
				if (!m_Buffer.empty())
				{
					ECSCore* pECS = ECSCore::GetInstance();
					auto* pCharacterColliderComponents = pECS->GetComponentArray<CharacterColliderComponent>();
					auto* pPositionComponents = pECS->GetComponentArray<PositionComponent>();
					auto* pNetPosComponents = pECS->GetComponentArray<NetworkPositionComponent>();
					auto* pVelocityComponents = pECS->GetComponentArray<VelocityComponent>();
					auto* pRotationComponents = pECS->GetComponentArray<RotationComponent>();

					const NetworkPositionComponent& netPosComponent		= pNetPosComponents->GetConstData(entityPlayer);
					const PositionComponent& constPositionComponent		= pPositionComponents->GetConstData(entityPlayer);
					VelocityComponent& velocityComponent				= pVelocityComponents->GetData(entityPlayer);
					const RotationComponent& constRotationComponent		= pRotationComponents->GetConstData(entityPlayer);

					for (const GameState& gameState : m_Buffer)
					{
						ASSERT(gameState.SimulationTick - 1 == m_CurrentGameState.SimulationTick);

						m_CurrentGameState = gameState;

						if (constRotationComponent.Quaternion != gameState.Rotation)
						{
							RotationComponent& rotationComponent = const_cast<RotationComponent&>(constRotationComponent);
							rotationComponent.Quaternion = gameState.Rotation;
							rotationComponent.Dirty = true;
						}

						PlayerActionSystem::ComputeVelocity(constRotationComponent.Quaternion, gameState.DeltaForward, gameState.DeltaLeft, velocityComponent.Velocity);
						CharacterControllerHelper::TickCharacterController(dt, entityPlayer, pCharacterColliderComponents, pNetPosComponents, pVelocityComponents);

						NetworkSegment* pPacket = m_pClient->GetFreePacket(NetworkSegment::TYPE_PLAYER_ACTION);
						BinaryEncoder encoder(pPacket);
						encoder.WriteInt32(entityPlayer);
						encoder.WriteInt32(m_CurrentGameState.SimulationTick);
						encoder.WriteVec3(netPosComponent.Position);
						encoder.WriteVec3(velocityComponent.Velocity);
						encoder.WriteQuat(constRotationComponent.Quaternion);
						m_pClient->SendReliableBroadcast(pPacket);

						if (constPositionComponent.Position != netPosComponent.Position)
						{
							PositionComponent& positionComponent = const_cast<PositionComponent&>(constPositionComponent);

							positionComponent.Position = gameState.Position;
							positionComponent.Dirty = true;
						}
					}

					m_Buffer.clear();
				}
			}
		}
	}

	void ClientRemoteSystem::OnConnecting(IClient* pClient)
	{
		m_pClient = (ClientRemoteBase*)pClient;

		ClientConnectingEvent event(pClient);
		EventQueue::SendEventImmediate(event);
	}

	void ClientRemoteSystem::OnConnected(IClient* pClient)
	{
		ClientConnectedEvent event(pClient);
		EventQueue::SendEventImmediate(event);
	}

	void ClientRemoteSystem::OnDisconnecting(IClient* pClient)
	{
		ClientDisconnectingEvent event(pClient);
		EventQueue::SendEventImmediate(event);
	}

	void ClientRemoteSystem::OnDisconnected(IClient* pClient)
	{
		ClientDisconnectedEvent event(pClient);
		EventQueue::SendEventImmediate(event);
	}

	void ClientRemoteSystem::OnPacketReceived(IClient* pClient, NetworkSegment* pPacket)
	{
		PacketReceivedEvent event(pClient, pPacket);
		EventQueue::SendEventImmediate(event);

		if (pPacket->GetType() == NetworkSegment::TYPE_PLAYER_ACTION)
		{
			GameState gameState = {};

			BinaryDecoder decoder(pPacket);
			decoder.ReadInt32(gameState.SimulationTick);
			decoder.ReadQuat(gameState.Rotation);
			decoder.ReadInt8(gameState.DeltaForward);
			decoder.ReadInt8(gameState.DeltaLeft);

			m_Buffer.insert(gameState);
		}
	}

	void ClientRemoteSystem::OnClientReleased(IClient* pClient)
	{
		UNREFERENCED_VARIABLE(pClient);
		delete this;
	}

	void ClientRemoteSystem::OnPacketDelivered(NetworkSegment* pPacket)
	{
		UNREFERENCED_VARIABLE(pPacket);
	}

	void ClientRemoteSystem::OnPacketResent(NetworkSegment* pPacket, uint8 retries)
	{
		UNREFERENCED_VARIABLE(pPacket);
		UNREFERENCED_VARIABLE(retries);
	}

	void ClientRemoteSystem::OnPacketMaxTriesReached(NetworkSegment* pPacket, uint8 retries)
	{
		UNREFERENCED_VARIABLE(pPacket);
		UNREFERENCED_VARIABLE(retries);
		m_pClient->Disconnect("ClientRemoteSystem::OnPacketMaxTriesReached()");
	}
}