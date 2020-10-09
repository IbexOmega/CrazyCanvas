#include "Game/World/Player/PlayerActionSystem.h"

#include "Game/ECS/Systems/Networking/ClientBaseSystem.h"

#include "Game/ECS/Components/Player/PlayerComponent.h"
#include "Game/ECS/Components/Physics/Transform.h"

#include "Game/Multiplayer/GameState.h"

#include "ECS/ECSCore.h"

#include "Input/API/Input.h"

#include "Engine/EngineLoop.h"

#include "Input/API/InputActionSystem.h"

#include "Application/API/CommonApplication.h"

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

		float32 dt = float32(deltaTime.AsSeconds());

		const ComponentArray<PlayerComponent>* pPlayerComponents	= pECS->GetComponentArray<PlayerComponent>();
		ComponentArray<RotationComponent>* pRotationComponents		= pECS->GetComponentArray<RotationComponent>();
		ComponentArray<VelocityComponent>* pVelocityComponents		= pECS->GetComponentArray<VelocityComponent>();

		const PlayerComponent& playerComponent = pPlayerComponents->GetData(entityPlayer);
		RotationComponent& rotationComponent = pRotationComponents->GetData(entityPlayer);
		VelocityComponent& velocityComponent = pVelocityComponents->GetData(entityPlayer);
		
		//Update Horizontal Movement
		{
			glm::i8vec2 deltaVelocity =
			{
				int8(InputActionSystem::IsActive("CAM_RIGHT") - InputActionSystem::IsActive("CAM_LEFT")),		// X: Right
				int8(InputActionSystem::IsActive("CAM_FORWARD") - InputActionSystem::IsActive("CAM_BACKWARD"))	// Y: Forward
			};
			
			ComputeVelocity(deltaVelocity.x, deltaVelocity.y, velocityComponent.Velocity);
		}

		//Update Rotational Movement
		{
			// Rotation from keyboard input. Applied later, after input from mouse has been read as well.
			float addedPitch	= dt * float(InputActionSystem::IsActive("CAM_ROT_UP") - InputActionSystem::IsActive("CAM_ROT_DOWN"));
			float addedYaw		= dt * float(InputActionSystem::IsActive("CAM_ROT_LEFT") - InputActionSystem::IsActive("CAM_ROT_RIGHT"));

			const float MAX_PITCH = glm::half_pi<float>() - 0.01f;

			glm::vec3 forward = GetForward(rotationComponent.Quaternion);
			float currentPitch = glm::clamp(GetPitch(forward) + addedPitch, -MAX_PITCH, MAX_PITCH);
			float currentYaw = GetYaw(forward) + addedYaw;

			rotationComponent.Quaternion =
				glm::angleAxis(currentYaw, g_DefaultUp) *		// Yaw
				glm::angleAxis(currentPitch, g_DefaultRight);	// Pitch
		}
	}

	void PlayerActionSystem::ComputeVelocity(int8 deltaForward, int8 deltaLeft, glm::vec3& result)
	{
		result.z = (float32)(1.0 * (float64)deltaForward);
		result.x = (float32)(1.0 * (float64)deltaLeft);
	}
}