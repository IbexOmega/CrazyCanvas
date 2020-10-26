#include "World/Player/Server/PlayerRemoteSystem.h"
#include "World/Player/CharacterControllerHelper.h"

#include "Game/ECS/Components/Physics/Collision.h"
#include "Game/ECS/Components/Player/PlayerComponent.h"

#include "World/Player/PlayerActionSystem.h"

#include "ECS/ECSCore.h"

#include "Multiplayer/Packet/PlayerAction.h"
#include "Multiplayer/Packet/PlayerActionResponse.h"

using namespace LambdaEngine;

PlayerRemoteSystem::PlayerRemoteSystem()
{
	
}

PlayerRemoteSystem::~PlayerRemoteSystem()
{
	
}

void PlayerRemoteSystem::Init()
{
	SystemRegistration systemReg = {};
	systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
	{
		{
			.pSubscriber = &m_Entities,
			.ComponentAccesses =
			{
				{NDA, PlayerBaseComponent::Type()},
				{RW, CharacterColliderComponent::Type()},
				{RW, NetworkPositionComponent::Type()},
				{R , PositionComponent::Type()},
				{RW, VelocityComponent::Type()},
				{RW, RotationComponent::Type()},
				{R, PacketComponent<PlayerAction>::Type()},
				{RW, PacketComponent<PlayerActionResponse>::Type()},
			}
		}
	};
	systemReg.Phase = 0;

	RegisterSystem(systemReg);
}

void PlayerRemoteSystem::FixedTickMainThread(LambdaEngine::Timestamp deltaTime)
{
	ECSCore* pECS = ECSCore::GetInstance();
	auto* pPlayerActionComponents = pECS->GetComponentArray<PacketComponent<PlayerAction>>();
	auto* pPlayerActionResponseComponents = pECS->GetComponentArray<PacketComponent<PlayerActionResponse>>();
	auto* pCharacterColliderComponents = pECS->GetComponentArray<CharacterColliderComponent>();
	auto* pPositionComponents = pECS->GetComponentArray<PositionComponent>();
	auto* pNetPosComponents = pECS->GetComponentArray<NetworkPositionComponent>();
	auto* pVelocityComponents = pECS->GetComponentArray<VelocityComponent>();
	auto* pRotationComponents = pECS->GetComponentArray<RotationComponent>();

	const float32 dt = (float32)deltaTime.AsSeconds();

	for(Entity entityPlayer : m_Entities)
	{
		const PacketComponent<PlayerAction>& playerActionComponent = pPlayerActionComponents->GetConstData(entityPlayer);
		PacketComponent<PlayerActionResponse>& playerActionResponseComponent = pPlayerActionResponseComponents->GetData(entityPlayer);
		const NetworkPositionComponent& netPosComponent = pNetPosComponents->GetConstData(entityPlayer);
		const PositionComponent& constPositionComponent = pPositionComponents->GetConstData(entityPlayer);
		VelocityComponent& velocityComponent = pVelocityComponents->GetData(entityPlayer);
		const RotationComponent& constRotationComponent = pRotationComponents->GetConstData(entityPlayer);

		const TArray<PlayerAction>& gameStates = playerActionComponent.GetPacketsReceived();

		for (const PlayerAction& gameState : gameStates)
		{
			PlayerAction& currentGameState = m_CurrentGameStates[entityPlayer];
			ASSERT(gameState.SimulationTick - 1 == currentGameState.SimulationTick);
			currentGameState = gameState;

			if (constRotationComponent.Quaternion != gameState.Rotation)
			{
				RotationComponent& rotationComponent = const_cast<RotationComponent&>(constRotationComponent);
				rotationComponent.Quaternion = gameState.Rotation;
				rotationComponent.Dirty = true;
			}

			PlayerActionSystem::ComputeVelocity(constRotationComponent.Quaternion, gameState.DeltaForward, gameState.DeltaLeft, velocityComponent.Velocity);
			CharacterControllerHelper::TickCharacterController(dt, entityPlayer, pCharacterColliderComponents, pNetPosComponents, pVelocityComponents);


			PlayerActionResponse packet;
			packet.SimulationTick	= gameState.SimulationTick;
			packet.Position			= netPosComponent.Position;
			packet.Velocity			= velocityComponent.Velocity;
			packet.Rotation			= constRotationComponent.Quaternion;
			playerActionResponseComponent.SendPacket(packet);


			if (constPositionComponent.Position != netPosComponent.Position)
			{
				PositionComponent& positionComponent = const_cast<PositionComponent&>(constPositionComponent);

				positionComponent.Position = netPosComponent.Position;
				positionComponent.Dirty = true;
			}
		}
	}
}