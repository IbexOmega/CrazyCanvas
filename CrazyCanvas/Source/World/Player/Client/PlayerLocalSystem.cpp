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
	m_PlayerActionSystem()
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

void PlayerLocalSystem::PlaySimulationTick(LambdaEngine::Timestamp deltaTime, float32 dt, PlayerGameState& clientState)
{
	if (!m_Entities.Empty())
	{
		Entity localPlayerEntity = m_Entities[0];

		//Get new packets
		ECSCore* pECS = ECSCore::GetInstance();
		const PacketComponent<PacketPlayerActionResponse>& pPacketComponent = pECS->GetComponent<PacketComponent<PacketPlayerActionResponse>>(localPlayerEntity);
		RegisterServerGameStates(pPacketComponent.GetPacketsReceived());

		TickLocalPlayerAction(deltaTime, localPlayerEntity, &clientState);

		if (!MultiplayerUtils::IsSingleplayer())
			SendGameState(clientState, localPlayerEntity);
	}
}

/*
* Called when to replay a specific simulation tick
*/
void PlayerLocalSystem::ReplayGameState(Timestamp deltaTime, float32 dt, PlayerGameState& clientState)
{
	Entity entityPlayer = m_Entities[0];

	ECSCore* pECS = ECSCore::GetInstance();

	ComponentArray<CharacterColliderComponent>* pCharacterColliderComponents = pECS->GetComponentArray<CharacterColliderComponent>();
	ComponentArray<NetworkPositionComponent>* pNetPosComponents = pECS->GetComponentArray<NetworkPositionComponent>();
	ComponentArray<VelocityComponent>* pVelocityComponents = pECS->GetComponentArray<VelocityComponent>();

	NetworkPositionComponent& netPosComponent = pNetPosComponents->GetData(entityPlayer);
	VelocityComponent& velocityComponent = pVelocityComponents->GetData(entityPlayer);

	/*
	* Returns the velocity based on key presses
	*/
	PlayerActionSystem::ComputeVelocity(clientState.Rotation, clientState.DeltaForward, clientState.DeltaLeft, velocityComponent.Velocity);

	/*
	* Sets the position of the PxController taken from the PositionComponent.
	* Move the PxController using the VelocityComponent by the deltatime.
	* Calculates a new Velocity based on the difference of the last position and the new one.
	* Sets the new position of the PositionComponent
	*/
	CharacterControllerHelper::TickCharacterController(dt, entityPlayer, pCharacterColliderComponents, pNetPosComponents, pVelocityComponents);

	clientState.Position = netPosComponent.Position;
	clientState.Velocity = velocityComponent.Velocity;
}

void PlayerLocalSystem::SurrenderGameState(const PacketPlayerActionResponse& serverState)
{
	Entity entityPlayer = m_Entities[0];

	ECSCore* pECS = ECSCore::GetInstance();

	NetworkPositionComponent& netPosComponent = pECS->GetComponent<NetworkPositionComponent>(entityPlayer);
	VelocityComponent& velocityComponent = pECS->GetComponent<VelocityComponent>(entityPlayer);

	netPosComponent.Position = serverState.Position;
	velocityComponent.Velocity = serverState.Velocity;
}

bool PlayerLocalSystem::CompareGamesStates(const PlayerGameState& clientState, const PacketPlayerActionResponse& serverState)
{
	bool result = true;
	if (glm::distance(clientState.Position, serverState.Position) > EPSILON)
	{
		LOG_ERROR("Prediction Error, Tick: %d, Position: [L: %f, %f, %f] [S: %f, %f, %f]", clientState.SimulationTick, clientState.Position.x, clientState.Position.y, serverState.Position.z, serverState.Position.x, serverState.Position.y, serverState.Position.z);
		result = false;
	}

	if (glm::distance(clientState.Velocity, serverState.Velocity) > EPSILON)
	{
		LOG_ERROR("Prediction Error, Tick: %d, Velocity: [L: %f, %f, %f] [S: %f, %f, %f]", clientState.SimulationTick, clientState.Velocity.x, clientState.Velocity.y, serverState.Velocity.z, serverState.Velocity.x, serverState.Velocity.y, serverState.Velocity.z);
		result = false;
	}

	return result;
}