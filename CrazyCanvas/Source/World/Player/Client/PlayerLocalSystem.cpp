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
	m_FramesToReconcile()
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

void PlayerLocalSystem::PlaySimulationTick(LambdaEngine::Timestamp deltaTime, float32 dt, int32 simulationTick)
{
	if (!m_Entities.Empty())
	{
		Entity localPlayerEntity = m_Entities[0];

		//Get new packets
		ECSCore* pECS = ECSCore::GetInstance();
		const PacketComponent<PacketPlayerActionResponse>& pPacketComponent = pECS->GetComponent<PacketComponent<PacketPlayerActionResponse>>(localPlayerEntity);
		const TArray<PacketPlayerActionResponse>& packets = pPacketComponent.GetPacketsReceived();
		m_FramesProcessedByServer.Insert(m_FramesProcessedByServer.end(), packets.begin(), packets.end());

		PlayerGameState gameState = {};
		gameState.SimulationTick = simulationTick;

		TickLocalPlayerAction(deltaTime, localPlayerEntity, &gameState);

		if (!MultiplayerUtils::IsSingleplayer())
			SendGameState(gameState, localPlayerEntity);
	}
}

/*
* Called when to replay a specific simulation tick
*/
void PlayerLocalSystem::ReplaySimulationTick(Timestamp deltaTime, float32 dt, uint32 i, int32 simulationTick)
{
	Entity entityPlayer = m_Entities[0];

	ECSCore* pECS = ECSCore::GetInstance();

	ComponentArray<CharacterColliderComponent>* pCharacterColliderComponents = pECS->GetComponentArray<CharacterColliderComponent>();
	ComponentArray<NetworkPositionComponent>* pNetPosComponents = pECS->GetComponentArray<NetworkPositionComponent>();
	ComponentArray<VelocityComponent>* pVelocityComponents = pECS->GetComponentArray<VelocityComponent>();

	NetworkPositionComponent& netPosComponent = pNetPosComponents->GetData(entityPlayer);
	VelocityComponent& velocityComponent = pVelocityComponents->GetData(entityPlayer);

	PlayerGameState& clientFrame = m_FramesToReconcile[i];

	ASSERT(clientFrame.SimulationTick == simulationTick);


	/*
	* Returns the velocity based on key presses
	*/
	PlayerActionSystem::ComputeVelocity(clientFrame.Rotation, clientFrame.DeltaForward, clientFrame.DeltaLeft, velocityComponent.Velocity);

	/*
	* Sets the position of the PxController taken from the PositionComponent.
	* Move the PxController using the VelocityComponent by the deltatime.
	* Calculates a new Velocity based on the difference of the last position and the new one.
	* Sets the new position of the PositionComponent
	*/
	CharacterControllerHelper::TickCharacterController(dt, entityPlayer, pCharacterColliderComponents, pNetPosComponents, pVelocityComponents);

	clientFrame.Position = netPosComponent.Position;
	clientFrame.Velocity = velocityComponent.Velocity;
}

/*
* Called on the SimulationTick that got a prediction error
*/
void PlayerLocalSystem::SurrenderGameState(int32 simulationTick)
{
	Entity entityPlayer = m_Entities[0];

	PacketPlayerActionResponse& serverFrame = m_FramesProcessedByServer[0];
	PlayerGameState& clientFrame = m_FramesToReconcile[0];

	ASSERT(serverFrame.SimulationTick == simulationTick);
	ASSERT(clientFrame.SimulationTick == simulationTick);

	ECSCore* pECS = ECSCore::GetInstance();

	NetworkPositionComponent& netPosComponent = pECS->GetComponent<NetworkPositionComponent>(entityPlayer);
	VelocityComponent& velocityComponent = pECS->GetComponent<VelocityComponent>(entityPlayer);

	netPosComponent.Position = serverFrame.Position;
	velocityComponent.Velocity = serverFrame.Velocity;
}

/*
* Returns the next SimulationTick that this system is ready for
*/
int32 PlayerLocalSystem::GetNextAvailableSimulationTick()
{
	if (m_FramesProcessedByServer.IsEmpty())
		return -1;

	return m_FramesProcessedByServer[0].SimulationTick;
}

/*
* Compares the local gamestate with the server gamestate
*/
bool PlayerLocalSystem::CompareNextGamesStates(int32 simulationTick)
{
	Entity entityPlayer = m_Entities[0];

	PacketPlayerActionResponse& serverFrame = m_FramesProcessedByServer[0];
	PlayerGameState& clientFrame = m_FramesToReconcile[0];

	ASSERT(serverFrame.SimulationTick == simulationTick);
	ASSERT(clientFrame.SimulationTick == simulationTick);

	return CompareGameStates(clientFrame, serverFrame);
}

/*
* Deletes the gamestate
*/
void PlayerLocalSystem::DeleteGameState(int32 simulationTick)
{
	PacketPlayerActionResponse& serverFrame = m_FramesProcessedByServer[0];
	PlayerGameState& clientFrame = m_FramesToReconcile[0];

	ASSERT(serverFrame.SimulationTick == simulationTick);
	ASSERT(clientFrame.SimulationTick == simulationTick);

	m_FramesProcessedByServer.Erase(m_FramesProcessedByServer.begin());
	m_FramesToReconcile.Erase(m_FramesToReconcile.begin());
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