#include "Game/World/Player/PlayerActionSystem.h"

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

	void PlayerActionSystem::TickMainThread(Timestamp deltaTime, Entity entityPlayer)
	{
		ECSCore* pECS = ECSCore::GetInstance();
		float32 dt = (float32)deltaTime.AsSeconds();

		ComponentArray<RotationComponent>* pRotationComponents = pECS->GetComponentArray<RotationComponent>();
		RotationComponent& rotationComponent = pRotationComponents->GetData(entityPlayer);

		// Rotation from keyboard input. Applied later, after input from mouse has been read as well.
		float addedPitch = dt * float(InputActionSystem::IsActive("CAM_ROT_UP") - InputActionSystem::IsActive("CAM_ROT_DOWN"));
		float addedYaw = dt * float(InputActionSystem::IsActive("CAM_ROT_LEFT") - InputActionSystem::IsActive("CAM_ROT_RIGHT"));

		const float MAX_PITCH = glm::half_pi<float>() - 0.01f;

		glm::vec3 forward = GetForward(rotationComponent.Quaternion);
		float currentPitch = glm::clamp(GetPitch(forward) + addedPitch, -MAX_PITCH, MAX_PITCH);
		float currentYaw = GetYaw(forward) + addedYaw;

		rotationComponent.Quaternion =
			glm::angleAxis(currentYaw, g_DefaultUp) *		// Yaw
			glm::angleAxis(currentPitch, g_DefaultRight);	// Pitch
	}

	void PlayerActionSystem::DoAction(Timestamp deltaTime, Entity entityPlayer, GameState* pGameState)
	{
		UNREFERENCED_VARIABLE(deltaTime);

		ECSCore* pECS = ECSCore::GetInstance();

		ComponentArray<RotationComponent>* pRotationComponents = pECS->GetComponentArray<RotationComponent>();
		ComponentArray<VelocityComponent>* pVelocityComponents = pECS->GetComponentArray<VelocityComponent>();

		RotationComponent& rotationComponent = pRotationComponents->GetData(entityPlayer);
		VelocityComponent& velocityComponent = pVelocityComponents->GetData(entityPlayer);

		glm::i8vec2 deltaVelocity =
		{
			int8(InputActionSystem::IsActive("CAM_RIGHT") - InputActionSystem::IsActive("CAM_LEFT")),		// X: Right
			int8(InputActionSystem::IsActive("CAM_BACKWARD") - InputActionSystem::IsActive("CAM_FORWARD"))	// Y: Forward
		};

		ComputeVelocity(rotationComponent.Quaternion, deltaVelocity.x, deltaVelocity.y, velocityComponent.Velocity);

		pGameState->DeltaForward = deltaVelocity.x;
		pGameState->DeltaLeft = deltaVelocity.y;
		pGameState->Rotation = rotationComponent.Quaternion;
	}

	void PlayerActionSystem::ComputeVelocity(const glm::quat& rotation, int8 deltaForward, int8 deltaLeft, glm::vec3& result)
	{
		if (deltaForward == 0 && deltaLeft == 0)
		{
			result = glm::vec3(0.0f);
			return;
		}

		float32 movespeed = 2.0f;
		result = rotation * glm::vec3(deltaForward, 0.0f, deltaLeft);
		result.y = 0.0f;
		result = glm::normalize(result);
		result *= movespeed;
	}
}
