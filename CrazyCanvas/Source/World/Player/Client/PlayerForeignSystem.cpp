#include "World/Player/Client/PlayerForeignSystem.h"
#include "World/Player/CharacterControllerHelper.h"

#include "Game/ECS/Components/Physics/Collision.h"
#include "Game/ECS/Components/Player/PlayerComponent.h"

#include "Game/Multiplayer/MultiplayerUtils.h"

#include "ECS/ECSCore.h"

#include "ECS/Components/Multiplayer/PacketComponent.h"

using namespace LambdaEngine;

PlayerForeignSystem::PlayerForeignSystem()
{
	
}

PlayerForeignSystem::~PlayerForeignSystem()
{
	
}

void PlayerForeignSystem::Init()
{
	SystemRegistration systemReg = {};
	systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
	{
		{
			.pSubscriber = &m_Entities,
			.ComponentAccesses =
			{
				{NDA, PlayerForeignComponent::Type()},
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

void PlayerForeignSystem::FixedTickMainThread(LambdaEngine::Timestamp deltaTime)
{
	ECSCore* pECS = ECSCore::GetInstance();
	float32 dt = (float32)deltaTime.AsSeconds();

	const ComponentArray<NetworkPositionComponent>* pNetPosComponents = pECS->GetComponentArray<NetworkPositionComponent>();
	ComponentArray<CharacterColliderComponent>* pCharacterColliders = pECS->GetComponentArray<CharacterColliderComponent>();
	ComponentArray<VelocityComponent>* pVelocityComponents = pECS->GetComponentArray<VelocityComponent>();
	const ComponentArray<PositionComponent>* pPositionComponents = pECS->GetComponentArray<PositionComponent>();
	ComponentArray<RotationComponent>* pRotationComponents = pECS->GetComponentArray<RotationComponent>();
	const ComponentArray<PacketComponent<PlayerActionResponse>>* pPacketComponents = pECS->GetComponentArray<PacketComponent<PlayerActionResponse>>();

	for (Entity entity : m_Entities)
	{
		const NetworkPositionComponent& constNetPosComponent = pNetPosComponents->GetConstData(entity);
		const PositionComponent& positionComponent = pPositionComponents->GetConstData(entity);
		VelocityComponent& velocityComponent = pVelocityComponents->GetData(entity);
		const PacketComponent<PlayerActionResponse>& packetComponent = pPacketComponents->GetConstData(entity);

		const TArray<PlayerActionResponse>& gameStates = packetComponent.GetPacketsReceived();

		if (!gameStates.IsEmpty())
		{
			for (const PlayerActionResponse& gameState : gameStates)
			{
				const RotationComponent& constRotationComponent = pRotationComponents->GetConstData(entity);

				if (constRotationComponent.Quaternion != gameState.Rotation)
				{
					RotationComponent& rotationComponent = const_cast<RotationComponent&>(constRotationComponent);
					rotationComponent.Quaternion = gameState.Rotation;
					rotationComponent.Dirty = true;
				}

				if (glm::any(glm::notEqual(constNetPosComponent.Position, gameState.Position)))
				{
					NetworkPositionComponent& netPosComponent = const_cast<NetworkPositionComponent&>(constNetPosComponent);
					netPosComponent.PositionLast = positionComponent.Position;
					netPosComponent.Position = gameState.Position;
					netPosComponent.TimestampStart = EngineLoop::GetTimeSinceStart();
					netPosComponent.Dirty = true;
				}

				velocityComponent.Velocity = gameState.Velocity;

				CharacterControllerHelper::TickForeignCharacterController(dt, entity, pCharacterColliders, pNetPosComponents, pVelocityComponents);
			}
		}
		else //Data does not exist for the current frame :(
		{
			velocityComponent.Velocity.y -= GRAVITATIONAL_ACCELERATION * dt;

			NetworkPositionComponent& netPosComponent = const_cast<NetworkPositionComponent&>(constNetPosComponent);
			netPosComponent.PositionLast = positionComponent.Position;
			netPosComponent.Position += velocityComponent.Velocity * dt;
			netPosComponent.TimestampStart = EngineLoop::GetTimeSinceStart();
			netPosComponent.Dirty = true;

			CharacterControllerHelper::TickForeignCharacterController(dt, entity, pCharacterColliders, pNetPosComponents, pVelocityComponents);
		}
	}
}