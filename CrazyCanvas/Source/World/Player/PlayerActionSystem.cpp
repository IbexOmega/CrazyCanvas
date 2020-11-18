#include "World/Player/PlayerActionSystem.h"

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
	ECSCore* pECS = ECSCore::GetInstance();
	float32 dt = (float32)deltaTime.AsSeconds();

	// Rotation from keyboard input. Applied later, after input from mouse has been read as well.
	float addedPitch = dt * float(InputActionSystem::IsActive(EAction::ACTION_CAM_ROT_UP) - InputActionSystem::IsActive(EAction::ACTION_CAM_ROT_DOWN));
	float addedYaw = dt * float(InputActionSystem::IsActive(EAction::ACTION_CAM_ROT_LEFT) - InputActionSystem::IsActive(EAction::ACTION_CAM_ROT_RIGHT));

	if (m_MouseEnabled && Input::GetCurrentInputmode() == EInputLayer::GAME)
	{
		const MouseState& mouseState = Input::GetMouseState(EInputLayer::GAME);

		TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();
		const int32 halfWidth		= int32(0.5f * float32(window->GetWidth()));
		const int32 halfHeight	= int32(0.5f * float32(window->GetHeight()));

		const glm::vec2 mouseDelta(mouseState.Position.x - halfWidth, mouseState.Position.y - halfHeight);

		if (glm::length(mouseDelta) > glm::epsilon<float>())
		{
			//Todo: Move this into some settings file
			constexpr const float MOUSE_SPEED_FACTOR = 0.35f;
			addedYaw -= MOUSE_SPEED_FACTOR * (float)mouseDelta.x * dt;
			addedPitch -= MOUSE_SPEED_FACTOR * (float)mouseDelta.y * dt;
		}

		CommonApplication::Get()->SetMousePosition(halfWidth, halfHeight);
	}

	if (glm::abs(addedPitch) > 0.0f || glm::abs(addedYaw) > 0.0f)
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

void PlayerActionSystem::ComputeVelocity(const glm::quat& rotation, int8 deltaForward, int8 deltaLeft, glm::vec3& result)
{
	if (!Match::HasBegun() || (deltaForward == 0 && deltaLeft == 0))
	{
		result.x = 0.0f;
		result.z = 0.0f;
		return;
	}

	float32 movespeed = 4.0f;
	glm::vec3 currentVelocity;
	currentVelocity		= rotation * glm::vec3(deltaForward, 0.0f, deltaLeft);
	currentVelocity.y	= 0.0f;
	currentVelocity		= glm::normalize(currentVelocity);
	currentVelocity		*= movespeed;

	result.x = currentVelocity.x;
	result.z = currentVelocity.z;
}

void PlayerActionSystem::SetMouseEnabled(bool isEnabled)
{
	m_MouseEnabled = isEnabled;
}
