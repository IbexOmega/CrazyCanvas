#include "Game/ECS/Systems/Actions/PlayerActionSystem.h"
#include "Game/ECS/Systems/Networking/ClientBaseSystem.h"
#include "Game/ECS/Systems/Physics/CharacterControllerSystem.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Physics/Collision.h"
#include "Game/ECS/Components/Networking/NetworkPositionComponent.h"

#include "ECS/ECSCore.h"

#include "Input/API/Input.h"

#include "Engine/EngineLoop.h"

namespace LambdaEngine
{
	PlayerActionSystem::PlayerActionSystem()
	{

	}

	PlayerActionSystem::~PlayerActionSystem()
	{

	}

	void PlayerActionSystem::Init()
	{

	}

	void PlayerActionSystem::DoLocalPlayerAction(Timestamp deltaTime, Entity entityPlayer, GameState* pGameState)
	{
		ECSCore* pECS = ECSCore::GetInstance();

		auto* pVelocityComponents = pECS->GetComponentArray<VelocityComponent>();

		int8 deltaForward = int8(Input::IsKeyDown(EKey::KEY_T) - Input::IsKeyDown(EKey::KEY_G));
		int8 deltaLeft = int8(Input::IsKeyDown(EKey::KEY_F) - Input::IsKeyDown(EKey::KEY_H));

		VelocityComponent& velocityComponent = pVelocityComponents->GetData(entityPlayer);
		ComputeVelocity(deltaForward, deltaLeft, velocityComponent.Velocity);

		pGameState->DeltaForward	= deltaForward;
		pGameState->DeltaLeft		= deltaLeft;
	}

	void PlayerActionSystem::ComputeVelocity(int8 deltaForward, int8 deltaLeft, glm::vec3& result)
	{
		result.z = (float32)(1.0 * (float64)deltaForward);
		result.x = (float32)(1.0 * (float64)deltaLeft);
	}

	void PlayerActionSystem::TickLocalPlayerAction(Timestamp deltaTime, Entity entityPlayer, GameState* pGameState)
	{
		ECSCore* pECS	= ECSCore::GetInstance();
		float32 dt		= (float32)deltaTime.AsSeconds();

		auto* pCharacterColliderComponents	= pECS->GetComponentArray<CharacterColliderComponent>();
		auto* pNetPosComponents				= pECS->GetComponentArray<NetworkPositionComponent>();
		auto* pVelocityComponents			= pECS->GetComponentArray<VelocityComponent>();
		auto* pPositionComponents			= pECS->GetComponentArray<PositionComponent>();

		NetworkPositionComponent& netPosComponent	= pNetPosComponents->GetData(entityPlayer);
		VelocityComponent& velocityComponent		= pVelocityComponents->GetData(entityPlayer);
		const PositionComponent& positionComponent	= pPositionComponents->GetData(entityPlayer);

		netPosComponent.PositionLast	= positionComponent.Position; //Lerpt from the current interpolated position (The rendered one)
		netPosComponent.TimestampStart	= EngineLoop::GetTimeSinceStart();

		DoLocalPlayerAction(deltaTime, entityPlayer, pGameState);

		CharacterControllerSystem::TickCharacterController(dt, entityPlayer, pCharacterColliderComponents, pNetPosComponents, pVelocityComponents);

		pGameState->Position = netPosComponent.Position;
		pGameState->Velocity = velocityComponent.Velocity;
	}

	void PlayerActionSystem::TickOtherPlayersAction(Timestamp deltaTime)
	{
		ECSCore* pECS = ECSCore::GetInstance();
		float32 dt = (float32)deltaTime.AsSeconds();

		auto* pNetPosComponents		= pECS->GetComponentArray<NetworkPositionComponent>();
		auto* pVelocityComponents	= pECS->GetComponentArray<VelocityComponent>();
		auto* pPositionComponents	= pECS->GetComponentArray<PositionComponent>();

		for (auto& pair : m_EntityOtherStates)
		{
			GameStateOther& gameState	= pair.second;
			Entity entity				= pair.first;

			NetworkPositionComponent& netPosComponent	= pNetPosComponents->GetData(entity);
			const PositionComponent& positionComponent	= pPositionComponents->GetData(entity);
			VelocityComponent& velocityComponent		= pVelocityComponents->GetData(entity);

			if (gameState.HasNewData) //Data does not exist for the current frame :(
			{
				gameState.HasNewData = false;

				netPosComponent.PositionLast	= positionComponent.Position;
				netPosComponent.Position		= gameState.Position;
				netPosComponent.TimestampStart	= EngineLoop::GetTimeSinceStart();

				velocityComponent.Velocity		= gameState.Velocity;
			}
			else //Data exist for the current frame :)
			{
				netPosComponent.PositionLast	= positionComponent.Position;
				netPosComponent.Position		+= velocityComponent.Velocity * dt;
				netPosComponent.TimestampStart	= EngineLoop::GetTimeSinceStart();
			}
		}
	}

	void PlayerActionSystem::OnPacketPlayerAction(Entity entity, const GameState* pGameState)
	{
		m_EntityOtherStates[entity] = { 
			pGameState->Position,
			pGameState->Velocity,
			true
		};
	}
}