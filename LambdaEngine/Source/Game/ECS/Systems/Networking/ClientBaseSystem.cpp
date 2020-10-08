#include "Game/ECS/Systems/Networking/ClientBaseSystem.h"

#include "ECS/ECSCore.h"

#include "Engine/EngineLoop.h"

namespace LambdaEngine
{
	ClientBaseSystem::ClientBaseSystem()
	{
		
	}

	ClientBaseSystem::~ClientBaseSystem()
	{

	}

	void ClientBaseSystem::Tick(Timestamp deltaTime)
	{
		UNREFERENCED_VARIABLE(deltaTime);
	}

	void ClientBaseSystem::PredictVelocity(int8 deltaForward, int8 deltaLeft, glm::vec3& result)
	{
		result.z = (float32)(1.0 * (float64)deltaForward);

		result.x = (float32)(1.0 * (float64)deltaLeft);
	}

	/*void ClientBaseSystem::PlayerUpdate(Entity entity, const GameState& gameState)
	{
		PlayerMovementSystem::GetInstance().Move(entity, EngineLoop::GetFixedTimestep(), gameState.DeltaForward, gameState.DeltaLeft);
	}*/
}