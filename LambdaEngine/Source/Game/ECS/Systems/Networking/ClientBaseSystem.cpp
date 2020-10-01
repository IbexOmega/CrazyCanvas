#include "Game/ECS/Systems/Networking/ClientBaseSystem.h"
#include "Game/ECS/Systems/Player/PlayerMovementSystem.h"

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

	void ClientBaseSystem::PlayerUpdate(Entity entity, const GameState& gameState)
	{
		PlayerMovementSystem::GetInstance().Move(entity, EngineLoop::GetFixedTimestep(), gameState.DeltaForward, gameState.DeltaLeft);
	}
}