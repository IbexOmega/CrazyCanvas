#include "World/Player/PlayerLocal.h"

#include "Game/Multiplayer/MultiplayerUtils.h"
#include "Game/Multiplayer/CharacterControllerHelper.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Physics/Collision.h"
#include "Game/ECS/Components/Networking/NetworkPositionComponent.h"
#include "Game/ECS/Components/Player/PlayerComponent.h"

#include "Networking/API/NetworkSegment.h"
#include "Networking/API/BinaryDecoder.h"
#include "Networking/API/BinaryEncoder.h"
#include "Networking/API/IClient.h"

#include "ECS/ECSCore.h"

#include "Input/API/Input.h"

#include "Engine/EngineLoop.h"

#include "Application/API/Events/EventQueue.h"
#include "Application/API/Events/NetworkEvents.h"

#include "Input/API/InputActionSystem.h"

#include "Multiplayer/PacketType.h"

#define EPSILON 0.01f

using namespace LambdaEngine;

PlayerLocal::PlayerLocal() :
	m_PlayerActionSystem(),
	m_FramesToReconcile(),
	m_SimulationTick(0),
	m_pClient(nullptr)
{
	EventQueue::RegisterEventHandler<ClientConnectedEvent>(this, &PlayerLocal::OnClientConnected);
	EventQueue::RegisterEventHandler<ClientDisconnectedEvent>(this, &PlayerLocal::OnClientDisconnected);
}

PlayerLocal::~PlayerLocal()
{
	EventQueue::UnregisterEventHandler<ClientConnectedEvent>(this, &PlayerLocal::OnClientConnected);
	EventQueue::RegisterEventHandler<ClientDisconnectedEvent>(this, &PlayerLocal::OnClientDisconnected);
}

void PlayerLocal::Init()
{
	SystemRegistration systemReg = {};
	systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
	{
		{
			.pSubscriber = &m_Entities,
			.ComponentAccesses =
			{
				{NDA, PlayerLocalComponent::Type()},
				{RW, CharacterColliderComponent::Type()},
				{RW, NetworkPositionComponent::Type()},
				{R , PositionComponent::Type()},
				{RW, VelocityComponent::Type()},
				{RW, RotationComponent::Type()},
				{R, PacketComponent<PlayerActionResponse>::Type()},
			}
		}
	};
	systemReg.Phase = 0;

	RegisterSystem(systemReg);
}

void PlayerLocal::TickMainThread(Timestamp deltaTime)
{
	ASSERT(m_Entities.Size() <= 1);

	if (!m_Entities.Empty())
	{
		m_PlayerActionSystem.TickMainThread(deltaTime, m_Entities[0]);
	}
}

void PlayerLocal::FixedTickMainThread(Timestamp deltaTime)
{
	if (!m_Entities.Empty())
	{
		Reconcile();

		PlayerGameState gameState = {};
		gameState.SimulationTick = m_SimulationTick++;

		TickLocalPlayerAction(deltaTime, m_Entities[0], &gameState);

		if (!MultiplayerUtils::IsSingleplayer())
			SendGameState(gameState);
	}
}

void PlayerLocal::SendGameState(const PlayerGameState& gameState)
{
	if (m_pClient)
	{
		PlayerAction packet		= {};
		packet.SimulationTick	= gameState.SimulationTick;
		packet.Rotation			= gameState.Rotation;
		packet.DeltaForward		= gameState.DeltaForward;
		packet.DeltaLeft		= gameState.DeltaLeft;

		NetworkSegment* pPacket = m_pClient->GetFreePacket(PacketType::PLAYER_ACTION);
		pPacket->Write(&packet);
		m_pClient->SendReliable(pPacket);
	}
}

void PlayerLocal::TickLocalPlayerAction(Timestamp deltaTime, Entity entityPlayer, PlayerGameState* pGameState)
{
	ECSCore* pECS = ECSCore::GetInstance();
	float32 dt = (float32)deltaTime.AsSeconds();

	ComponentArray<CharacterColliderComponent>* pCharacterColliderComponents = pECS->GetComponentArray<CharacterColliderComponent>();
	ComponentArray<NetworkPositionComponent>* pNetPosComponents = pECS->GetComponentArray<NetworkPositionComponent>();
	ComponentArray<VelocityComponent>* pVelocityComponents = pECS->GetComponentArray<VelocityComponent>();
	const ComponentArray<PositionComponent>* pPositionComponents = pECS->GetComponentArray<PositionComponent>();

	const NetworkPositionComponent& netPosComponent = pNetPosComponents->GetConstData(entityPlayer);
	const VelocityComponent& velocityComponent = pVelocityComponents->GetConstData(entityPlayer);
	const PositionComponent& positionComponent = pPositionComponents->GetConstData(entityPlayer);

	NetworkPositionComponent& mutableNetPosComponent = const_cast<NetworkPositionComponent&>(netPosComponent);
	mutableNetPosComponent.PositionLast = positionComponent.Position; //Lerpt from the current interpolated position (The rendered one)
	mutableNetPosComponent.TimestampStart = EngineLoop::GetTimeSinceStart();

	DoAction(deltaTime, entityPlayer, pGameState);

	CharacterControllerHelper::TickCharacterController(dt, entityPlayer, pCharacterColliderComponents, pNetPosComponents, pVelocityComponents);

	pGameState->Position = netPosComponent.Position;
	pGameState->Velocity = velocityComponent.Velocity;

	m_FramesToReconcile.PushBack(*pGameState);
}

void PlayerLocal::DoAction(Timestamp deltaTime, Entity entityPlayer, PlayerGameState* pGameState)
{
	UNREFERENCED_VARIABLE(deltaTime);

	ECSCore* pECS = ECSCore::GetInstance();

	glm::i8vec2 deltaVelocity =
	{
		int8(InputActionSystem::IsActive("CAM_RIGHT") - InputActionSystem::IsActive("CAM_LEFT")),		// X: Right
		int8(InputActionSystem::IsActive("CAM_BACKWARD") - InputActionSystem::IsActive("CAM_FORWARD"))	// Y: Forward
	};

	const ComponentArray<RotationComponent>* pRotationComponents = pECS->GetComponentArray<RotationComponent>();
	ComponentArray<VelocityComponent>* pVelocityComponents = pECS->GetComponentArray<VelocityComponent>();

	const RotationComponent& rotationComponent = pRotationComponents->GetConstData(entityPlayer);
	VelocityComponent& velocityComponent = pVelocityComponents->GetData(entityPlayer);

	PlayerActionSystem::ComputeVelocity(rotationComponent.Quaternion, deltaVelocity.x, deltaVelocity.y, velocityComponent.Velocity);

	pGameState->DeltaForward = deltaVelocity.x;
	pGameState->DeltaLeft = deltaVelocity.y;
	pGameState->Rotation = rotationComponent.Quaternion;
}

bool PlayerLocal::OnClientConnected(const LambdaEngine::ClientConnectedEvent& event)
{
	m_pClient = event.pClient;
	return false;
}

bool PlayerLocal::OnClientDisconnected(const LambdaEngine::ClientDisconnectedEvent& event)
{
	m_pClient = nullptr;
	return false;
}

//Packets in the packet list must be cleard at the end of each fixed tick. Since some ticks may not have a packet and they are therfore reused.....

void PlayerLocal::Reconcile()
{
	ECSCore* pECS = ECSCore::GetInstance();
	const PacketComponent<PlayerActionResponse>& pPacketComponents = pECS->GetComponent<PacketComponent<PlayerActionResponse>>(m_Entities[0]);
	const TArray<PlayerActionResponse>& m_FramesProcessedByServer = pPacketComponents.PacketsReceived;

	for (int32 i = 0; i < m_FramesProcessedByServer.GetSize(); i++)
	{
		ASSERT(m_FramesProcessedByServer[i].SimulationTick == m_FramesToReconcile[0].SimulationTick);

		if (!CompareGameStates(m_FramesToReconcile[0], m_FramesProcessedByServer[i]))
		{
			ReplayGameStatesBasedOnServerGameState(m_FramesToReconcile.GetData(), m_FramesToReconcile.GetSize(), m_FramesProcessedByServer[i]);
		}

		m_FramesToReconcile.Erase(m_FramesToReconcile.Begin());
	}
}

void PlayerLocal::ReplayGameStatesBasedOnServerGameState(PlayerGameState* pGameStates, uint32 count, const PlayerActionResponse& gameStateServer)
{
	ECSCore* pECS = ECSCore::GetInstance();

	Entity entityPlayer = MultiplayerUtils::GetEntityPlayer(m_pClient);

	ComponentArray<CharacterColliderComponent>* pCharacterColliderComponents = pECS->GetComponentArray<CharacterColliderComponent>();
	ComponentArray<NetworkPositionComponent>* pNetPosComponents = pECS->GetComponentArray<NetworkPositionComponent>();
	ComponentArray<VelocityComponent>* pVelocityComponents = pECS->GetComponentArray<VelocityComponent>();

	NetworkPositionComponent& netPosComponent = pNetPosComponents->GetData(entityPlayer);
	VelocityComponent& velocityComponent = pVelocityComponents->GetData(entityPlayer);

	netPosComponent.Position = gameStateServer.Position;
	velocityComponent.Velocity = gameStateServer.Velocity;

	//Replay all game states since the game state which resulted in prediction ERROR

	// TODO: Rollback other entities not just the player 

	const Timestamp deltaTime = EngineLoop::GetFixedTimestep();
	const float32 dt = (float32)deltaTime.AsSeconds();

	for (uint32 i = 1; i < count; i++)
	{
		PlayerGameState& gameState = pGameStates[i];

		/*
		* Returns the velocity based on key presses
		*/
		PlayerActionSystem::ComputeVelocity(gameState.Rotation, gameState.DeltaForward, gameState.DeltaLeft, velocityComponent.Velocity);

		/*
		* Sets the position of the PxController taken from the PositionComponent.
		* Move the PxController using the VelocityComponent by the deltatime.
		* Calculates a new Velocity based on the difference of the last position and the new one.
		* Sets the new position of the PositionComponent
		*/
		CharacterControllerHelper::TickCharacterController(dt, entityPlayer, pCharacterColliderComponents, pNetPosComponents, pVelocityComponents);

		gameState.Position = netPosComponent.Position;
		gameState.Velocity = velocityComponent.Velocity;
	}
}

bool PlayerLocal::CompareGameStates(const PlayerGameState& gameStateLocal, const PlayerActionResponse& gameStateServer)
{
	bool result = true;
	if (glm::distance(gameStateLocal.Position, gameStateServer.Position) > EPSILON)
	{
		LOG_ERROR("Prediction Error, Tick: %d, Position: [L: %f, %f, %f] [S: %f, %f, %f]", gameStateLocal.SimulationTick, gameStateLocal.Position.x, gameStateLocal.Position.y, gameStateLocal.Position.z, gameStateServer.Position.x, gameStateServer.Position.y, gameStateServer.Position.z);
		result = false;
	}

	if (glm::distance(gameStateLocal.Velocity, gameStateServer.Velocity) > EPSILON)
	{
		LOG_ERROR("Prediction Error, Tick: %d, Velocity: [L: %f, %f, %f] [S: %f, %f, %f]", gameStateLocal.SimulationTick, gameStateLocal.Velocity.x, gameStateLocal.Velocity.y, gameStateLocal.Velocity.z, gameStateServer.Velocity.x, gameStateServer.Velocity.y, gameStateServer.Velocity.z);
		result = false;
	}

	return result;
}