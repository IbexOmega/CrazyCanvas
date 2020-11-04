#include "World/Player/Client/PlayerLocalSystem.h"
#include "World/Player/CharacterControllerHelper.h"

#include "Game/Multiplayer/MultiplayerUtils.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Physics/Collision.h"
#include "Game/ECS/Components/Networking/NetworkPositionComponent.h"
#include "Game/ECS/Components/Networking/NetworkComponent.h"
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

#include "Multiplayer/Packet/PacketType.h"
#include "Multiplayer/Packet/PacketPlayerAction.h"

#define EPSILON 0.01f

using namespace LambdaEngine;

PlayerLocalSystem::PlayerLocalSystem() :
	m_PlayerActionSystem(),
	m_FramesToReconcile(),
	m_SimulationTick(0)
{

}

void PlayerLocalSystem::Init()
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
				{RW, PacketComponent<PacketPlayerAction>::Type()},
				{R, PacketComponent<PacketPlayerActionResponse>::Type()},
			}
		}
	};
	systemReg.Phase = 0;

	RegisterSystem(TYPE_NAME(PlayerLocalSystem), systemReg);
}

void PlayerLocalSystem::TickMainThread(Timestamp deltaTime)
{
	ASSERT(m_Entities.Size() <= 1);

	if (!m_Entities.Empty())
	{
		m_PlayerActionSystem.TickMainThread(deltaTime, m_Entities[0]);
	}
}

void PlayerLocalSystem::FixedTickMainThread(Timestamp deltaTime)
{
	if (!m_Entities.Empty())
	{
		Entity localPlayerEntity = m_Entities[0];

		Reconcile(localPlayerEntity);

		PlayerGameState gameState = {};
		gameState.SimulationTick = m_SimulationTick++;

		TickLocalPlayerAction(deltaTime, localPlayerEntity, &gameState);

		if (!MultiplayerUtils::IsSingleplayer())
			SendGameState(gameState, localPlayerEntity);
	}
}

void PlayerLocalSystem::SendGameState(const PlayerGameState& gameState, Entity entityPlayer)
{
	ECSCore* pECS = ECSCore::GetInstance();
	PacketComponent<PacketPlayerAction>& pPacketComponent = pECS->GetComponent<PacketComponent<PacketPlayerAction>>(entityPlayer);

	PacketPlayerAction packet		= {};
	packet.SimulationTick	= gameState.SimulationTick;
	packet.Rotation			= gameState.Rotation;
	packet.DeltaForward		= gameState.DeltaForward;
	packet.DeltaLeft		= gameState.DeltaLeft;

	pPacketComponent.SendPacket(packet);
}

void PlayerLocalSystem::TickLocalPlayerAction(Timestamp deltaTime, Entity entityPlayer, PlayerGameState* pGameState)
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

void PlayerLocalSystem::DoAction(Timestamp deltaTime, Entity entityPlayer, PlayerGameState* pGameState)
{
	UNREFERENCED_VARIABLE(deltaTime);

	ECSCore* pECS = ECSCore::GetInstance();

	glm::i8vec2 deltaVelocity =
	{
		int8(InputActionSystem::IsActive(EAction::ACTION_MOVE_RIGHT) - InputActionSystem::IsActive(EAction::ACTION_MOVE_LEFT)),		// X: Right
		int8(InputActionSystem::IsActive(EAction::ACTION_MOVE_BACKWARD) - InputActionSystem::IsActive(EAction::ACTION_MOVE_FORWARD))	// Y: Forward
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

void PlayerLocalSystem::Reconcile(Entity entityPlayer)
{
	ECSCore* pECS = ECSCore::GetInstance();
	const PacketComponent<PacketPlayerActionResponse>& pPacketComponent = pECS->GetComponent<PacketComponent<PacketPlayerActionResponse>>(entityPlayer);
	const TArray<PacketPlayerActionResponse>& m_FramesProcessedByServer = pPacketComponent.GetPacketsReceived();

	for (uint32 i = 0; i < m_FramesProcessedByServer.GetSize(); i++)
	{
		ASSERT(m_FramesProcessedByServer[i].SimulationTick == m_FramesToReconcile[0].SimulationTick);

		if (!CompareGameStates(m_FramesToReconcile[0], m_FramesProcessedByServer[i]))
		{
			ReplayGameStatesBasedOnServerGameState(entityPlayer, m_FramesToReconcile.GetData(), m_FramesToReconcile.GetSize(), m_FramesProcessedByServer[i]);
		}

		m_FramesToReconcile.Erase(m_FramesToReconcile.Begin());
	}
}

void PlayerLocalSystem::ReplayGameStatesBasedOnServerGameState(Entity entityPlayer, PlayerGameState* pGameStates, uint32 count, const PacketPlayerActionResponse& gameStateServer)
{
	ECSCore* pECS = ECSCore::GetInstance();

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

bool PlayerLocalSystem::CompareGameStates(const PlayerGameState& gameStateLocal, const PacketPlayerActionResponse& gameStateServer)
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