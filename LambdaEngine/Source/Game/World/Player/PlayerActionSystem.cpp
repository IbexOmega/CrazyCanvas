#include "Game/World/Player/PlayerActionSystem.h"

#include "Game/ECS/Systems/Networking/ClientBaseSystem.h"

#include "Game/ECS/Components/Physics/Transform.h"

#include "Game/Multiplayer/GameState.h"

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

	void PlayerActionSystem::DoAction(Timestamp deltaTime, Entity entityPlayer, GameState* pGameState)
	{
		ECSCore* pECS = ECSCore::GetInstance();

		auto* pVelocityComponents = pECS->GetComponentArray<VelocityComponent>();

		int8 deltaForward	= int8(Input::IsKeyDown(EKey::KEY_T) - Input::IsKeyDown(EKey::KEY_G));
		int8 deltaLeft		= int8(Input::IsKeyDown(EKey::KEY_F) - Input::IsKeyDown(EKey::KEY_H));

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
}