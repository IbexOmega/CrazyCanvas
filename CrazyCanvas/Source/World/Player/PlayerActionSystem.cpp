#include "World/Player/PlayerActionSystem.h"
#include "World/Player/PlayerSettings.h"

#include "Game/ECS/Components/Player/PlayerComponent.h"
#include "Game/ECS/Components/Physics/Transform.h"

#include "ECS/ECSCore.h"

#include "Input/API/Input.h"

#include "Engine/EngineLoop.h"

#include "Input/API/InputActionSystem.h"

#include "Application/API/CommonApplication.h"
#include "Application/API/Events/EventQueue.h"


#include "Match/Match.h"

using namespace LambdaEngine;


bool PlayerActionSystem::m_MouseEnabled = true;

PlayerActionSystem::PlayerActionSystem()
{
	EventQueue::RegisterEventHandler<KeyPressedEvent>(this, &PlayerActionSystem::OnKeyPressed);
}

PlayerActionSystem::~PlayerActionSystem()
{
	EventQueue::UnregisterEventHandler<KeyPressedEvent>(this, &PlayerActionSystem::OnKeyPressed);
}

void PlayerActionSystem::Init()
{

}

void PlayerActionSystem::TickMainThread(Timestamp deltaTime, Entity entityPlayer)
{
	UNREFERENCED_VARIABLE(deltaTime);

	ECSCore* pECS = ECSCore::GetInstance();

	// Rotation from keyboard input. Applied later, after input from mouse has been read as well.
	float addedPitch = float(InputActionSystem::IsActive(EAction::ACTION_CAM_ROT_UP) - InputActionSystem::IsActive(EAction::ACTION_CAM_ROT_DOWN));
	float addedYaw = float(InputActionSystem::IsActive(EAction::ACTION_CAM_ROT_LEFT) - InputActionSystem::IsActive(EAction::ACTION_CAM_ROT_RIGHT));

	EInputLayer currentInputLayer = Input::GetCurrentInputmode();

	if (m_MouseEnabled && (currentInputLayer == EInputLayer::GAME || currentInputLayer  == EInputLayer::DEAD))
	{
		const MouseState& mouseState = Input::GetMouseState(currentInputLayer == EInputLayer::GAME ? EInputLayer::GAME : EInputLayer::DEAD);

		TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();
		const int32 halfWidth		= int32(0.5f * float32(window->GetWidth()));
		const int32 halfHeight	= int32(0.5f * float32(window->GetHeight()));

		const glm::vec2 mouseDelta(mouseState.Position.x - halfWidth, mouseState.Position.y - halfHeight);

		if (glm::length(mouseDelta) > glm::epsilon<float>())
		{
			addedYaw -= InputActionSystem::GetLookSensitivity() * (float)mouseDelta.x;
			addedPitch -= InputActionSystem::GetLookSensitivity() * (float)mouseDelta.y;
		}

		CommonApplication::Get()->SetMousePosition(halfWidth, halfHeight);
	}

	if ((glm::abs(addedPitch) > 0.0f || glm::abs(addedYaw) > 0.0f) && CommonApplication::Get()->GetMainWindow()->IsActiveWindow())
	{
		ComponentArray<RotationComponent>* pRotationComponents = pECS->GetComponentArray<RotationComponent>();
		RotationComponent& rotationComponent = pRotationComponents->GetData(entityPlayer);

		const float MAX_PITCH = glm::half_pi<float>() - 0.01f;

		glm::vec3 forward = GetForward(rotationComponent.Quaternion);
		float currentPitch = glm::clamp(GetPitch(forward) + addedPitch, -MAX_PITCH, MAX_PITCH);
		float currentYaw = GetYaw(forward) + addedYaw;

		rotationComponent.Quaternion =
			glm::angleAxis(currentYaw, g_DefaultUp) *		// Yaw
			glm::angleAxis(currentPitch, g_DefaultRight);	// Pitch
	}
}

bool PlayerActionSystem::OnKeyPressed(const KeyPressedEvent& event)
{
	if (event.Key == EKey::KEY_KEYPAD_0 || event.Key == EKey::KEY_END)
	{
		m_MouseEnabled = !m_MouseEnabled;
		CommonApplication::Get()->SetMouseVisibility(!m_MouseEnabled);
	}

	return false;
}

void PlayerActionSystem::ComputeVelocity(const glm::quat& rotation, const glm::i8vec3& deltaAction, bool walking, float32 dt, glm::vec3& velocity)
{
	bool horizontalMovement = deltaAction.x != 0 || deltaAction.z != 0;
	bool verticalMovement = deltaAction.y != 0;

	if (!Match::HasBegun())
	{
		velocity.x = 0.0f;
		velocity.z = 0.0f;
		return;
	}

	if (horizontalMovement)
	{
		glm::quat rotationNoPitch = rotation;
		rotationNoPitch.x = 0.0f;
		rotationNoPitch.z = 0.0f;
		rotationNoPitch = glm::normalize(rotationNoPitch);

		glm::vec3 currentVelocity;
		currentVelocity		= rotationNoPitch * glm::vec3(deltaAction.x, 0.0f, deltaAction.z);
		currentVelocity.y	= 0.0f;
		currentVelocity		= glm::normalize(currentVelocity);
		currentVelocity		*= (PLAYER_WALK_MOVEMENT_SPEED * float32(walking)) + (PLAYER_RUN_MOVEMENT_SPEED * float32(!walking));

		velocity.x = currentVelocity.x;
		velocity.z = currentVelocity.z;
	}
	else
	{
		float32 relativeVelocity = 1.0f / (1.0f + PLAYER_DRAG * dt);
		velocity.x *= relativeVelocity;
		velocity.z *= relativeVelocity;
	}

	if (verticalMovement)
	{
		velocity.y = velocity.y * float32(1 - deltaAction.y) + PLAYER_JUMP_SPEED * float32(deltaAction.y);
	}
}

void PlayerActionSystem::SetMouseEnabled(bool isEnabled)
{
	m_MouseEnabled = isEnabled;
}
