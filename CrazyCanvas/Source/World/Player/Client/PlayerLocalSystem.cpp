#include "World/Player/Client/PlayerLocalSystem.h"
#include "World/Player/CharacterControllerHelper.h"

#include "Game/Multiplayer/MultiplayerUtils.h"

#include "Game/ECS/Components/Networking/NetworkComponent.h"
#include "Game/ECS/Components/Player/PlayerComponent.h"
#include "Game/ECS/Components/Misc/InheritanceComponent.h"
#include "ECS/Components/Match/FlagComponent.h"

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

#include "World/Player/Client/PlayerSoundHelper.h"

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
		},
		{
			.pSubscriber = &m_FlagEntities,
			.ComponentAccesses =
			{
				{R, FlagComponent::Type()},
				{R, ParentComponent::Type()},
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
	packet.DeltaAction		= gameState.DeltaAction;
	packet.Walking			= gameState.Walking;
	packet.HoldingFlag		= gameState.HoldingFlag;

	pPacketComponent.SendPacket(packet);
}

void PlayerLocalSystem::TickLocalPlayerAction(float32 dt, Entity entityPlayer, PlayerGameState* pGameState)
{
	ECSCore* pECS = ECSCore::GetInstance();

	CharacterColliderComponent& characterColliderComponent		= pECS->GetComponent<CharacterColliderComponent>(entityPlayer);
	const NetworkPositionComponent& networkPositionComponent	= pECS->GetConstComponent<NetworkPositionComponent>(entityPlayer);
	VelocityComponent& velocityComponent						= pECS->GetComponent<VelocityComponent>(entityPlayer);
	const PositionComponent& positionComponent					= pECS->GetConstComponent<PositionComponent>(entityPlayer);
	const RotationComponent& rotationComponent					= pECS->GetConstComponent<RotationComponent>(entityPlayer);
	AudibleComponent& audibleComponent							= pECS->GetComponent<AudibleComponent>(entityPlayer);

	NetworkPositionComponent& mutableNetPosComponent = const_cast<NetworkPositionComponent&>(networkPositionComponent);
	mutableNetPosComponent.PositionLast = positionComponent.Position; //Lerpt from the current interpolated position (The rendered one)
	mutableNetPosComponent.TimestampStart = EngineLoop::GetTimeSinceStart();

	DoAction(dt, velocityComponent, audibleComponent, characterColliderComponent, rotationComponent, pGameState);

	CharacterControllerHelper::TickCharacterController(dt, characterColliderComponent, networkPositionComponent, velocityComponent);

	pGameState->Position = networkPositionComponent.Position;
	pGameState->Velocity = velocityComponent.Velocity;
}

void PlayerLocalSystem::DoAction(
	float32 dt,
	LambdaEngine::VelocityComponent& velocityComponent,
	LambdaEngine::AudibleComponent& audibleComponent,
	LambdaEngine::CharacterColliderComponent& characterColliderComponent,
	const LambdaEngine::RotationComponent& rotationComponent,
	PlayerGameState* pGameState)
{
	physx::PxControllerState playerControllerState;
	characterColliderComponent.pController->getState(playerControllerState);
	bool inAir = playerControllerState.touchedShape == nullptr;

	glm::i8vec3 deltaAction =
	{
		int8(InputActionSystem::IsActive(EAction::ACTION_MOVE_RIGHT) - InputActionSystem::IsActive(EAction::ACTION_MOVE_LEFT)),			// X: Right
		int8(InputActionSystem::IsActive(EAction::ACTION_MOVE_JUMP)) * int8(!inAir),													// Y: Up
		int8(InputActionSystem::IsActive(EAction::ACTION_MOVE_BACKWARD) - InputActionSystem::IsActive(EAction::ACTION_MOVE_FORWARD)),	// Z: Forward
	};

	bool walking = InputActionSystem::IsActive(EAction::ACTION_MOVE_WALK);

	ECSCore* pECS = ECSCore::GetInstance();
	auto pParentComponents = pECS->GetComponentArray<ParentComponent>();
	bool holdingFlag = false;
	for (Entity flagEntity : m_FlagEntities)
	{
		const ParentComponent& parentComponent = pParentComponents->GetConstData(flagEntity);
		if (parentComponent.Parent == m_Entities[0])
		{
			holdingFlag = true;
			break;
		}
	}
	PlayerActionSystem::ComputeVelocity(rotationComponent.Quaternion, deltaAction, walking, dt, velocityComponent.Velocity, holdingFlag);
	PlayerSoundHelper::HandleMovementSound(velocityComponent, audibleComponent, deltaAction, walking, inAir);

	pGameState->DeltaAction		= deltaAction;
	pGameState->Walking			= walking;
	pGameState->HoldingFlag		= holdingFlag;
	pGameState->Rotation		= rotationComponent.Quaternion;
}

void PlayerLocalSystem::PlaySimulationTick(float32 dt, PlayerGameState& clientState)
{
	UNREFERENCED_VARIABLE(dt);

	if (!m_Entities.Empty())
	{
		Entity localPlayerEntity = m_Entities[0];

		//Get new packets
		ECSCore* pECS = ECSCore::GetInstance();
		const PacketComponent<PacketPlayerActionResponse>& pPacketComponent = pECS->GetComponent<PacketComponent<PacketPlayerActionResponse>>(localPlayerEntity);
		RegisterServerGameStates(pPacketComponent.GetPacketsReceived());

		TickLocalPlayerAction(dt, localPlayerEntity, &clientState);

		if (!MultiplayerUtils::IsSingleplayer())
			SendGameState(clientState, localPlayerEntity);
	}
}

/*
* Called when to replay a specific simulation tick
*/
void PlayerLocalSystem::ReplayGameState(float32 dt, PlayerGameState& clientState)
{
	Entity entityPlayer = m_Entities[0];

	ECSCore* pECS = ECSCore::GetInstance();

	CharacterColliderComponent& characterColliderComponent	= pECS->GetComponent<CharacterColliderComponent>(entityPlayer);
	NetworkPositionComponent& networkPositionComponent		= pECS->GetComponent<NetworkPositionComponent>(entityPlayer);
	VelocityComponent& velocityComponent					= pECS->GetComponent<VelocityComponent>(entityPlayer);

	/*
	* Returns the velocity based on key presses
	*/
	PlayerActionSystem::ComputeVelocity(clientState.Rotation, clientState.DeltaAction, clientState.Walking, dt, velocityComponent.Velocity, clientState.HoldingFlag);

	/*
	* Sets the position of the PxController taken from the PositionComponent.
	* Move the PxController using the VelocityComponent by the deltatime.
	* Calculates a new Velocity based on the difference of the last position and the new one.
	* Sets the new position of the PositionComponent
	*/
	CharacterControllerHelper::TickCharacterController(dt, characterColliderComponent, networkPositionComponent, velocityComponent);

	clientState.Position = networkPositionComponent.Position;
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