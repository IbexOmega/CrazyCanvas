#include "World/Player/PlayerActionSystem.h"
#include "World/Player/PlayerSettings.h"
#include "World/SessionSettings.h"

#include "Game/ECS/Components/Player/PlayerComponent.h"
#include "Game/ECS/Components/Physics/Transform.h"

#include "ECS/ECSCore.h"

#include "Input/API/Input.h"

#include "Engine/EngineLoop.h"

#include "Input/API/InputActionSystem.h"

#include "Application/API/CommonApplication.h"
#include "Application/API/Events/EventQueue.h"

#include "ECS/Components/Match/FlagComponent.h"

#include "Game/ECS/Components/Misc/InheritanceComponent.h"

#include "Match/Match.h"

using namespace LambdaEngine;


bool PlayerActionSystem::m_MouseEnabled = true;
float32 PlayerActionSystem::m_Speed = 1.0f;

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

	// Rotation from keyboard input. Applied later, after input from mouse has been read as well.
	float addedPitch = float(InputActionSystem::IsActive(EAction::ACTION_CAM_ROT_UP) - InputActionSystem::IsActive(EAction::ACTION_CAM_ROT_DOWN));
	float addedYaw = float(InputActionSystem::IsActive(EAction::ACTION_CAM_ROT_LEFT) - InputActionSystem::IsActive(EAction::ACTION_CAM_ROT_RIGHT));

	const EInputLayer currentInputLayer = Input::GetCurrentInputmode();

	if (m_MouseEnabled && (currentInputLayer == EInputLayer::GAME || currentInputLayer  == EInputLayer::DEAD))
	{
		const MouseState& mouseState = Input::GetMouseState(currentInputLayer == EInputLayer::GAME ? EInputLayer::GAME : EInputLayer::DEAD);

		TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();
		const int32 halfWidth	= int32(0.5f * float32(window->GetWidth()));
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
		ECSCore* pECS = ECSCore::GetInstance();
		ComponentArray<RotationComponent>* pRotationComponents = pECS->GetComponentArray<RotationComponent>();
		RotationComponent& rotationComponent = pRotationComponents->GetData(entityPlayer);

		const float MAX_PITCH = glm::half_pi<float>() - 0.01f;

		const glm::vec3 forward = GetForward(rotationComponent.Quaternion);
		const float currentPitch = glm::clamp(GetPitch(forward) + addedPitch, -MAX_PITCH, MAX_PITCH);
		const float currentYaw = GetYaw(forward) + addedYaw;

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

void PlayerActionSystem::ComputeVelocity(const glm::quat& rotation, const glm::i8vec3& deltaAction, bool walking, float32 dt, glm::vec3& velocity, bool isHoldingFlag, float acceleration, float maxVelocity)
{
	bool verticalMovement = deltaAction.y != 0;

	if (isHoldingFlag)
		m_Speed = 0.8f;
	else
		m_Speed = 1.0f;

	if (!Match::HasBegun())
	{
		velocity.x = 0.0f;
		velocity.z = 0.0f;
		return;
	}

	glm::vec3 dir = glm::vec3(deltaAction.x, 0.0f, deltaAction.z);
	if (glm::length2(dir) > glm::epsilon<float>())
	{
		glm::quat rotationNoPitch = rotation;
		rotationNoPitch.x = 0.0f;
		rotationNoPitch.z = 0.0f;
		rotationNoPitch = glm::normalize(rotationNoPitch);

		dir = glm::normalize(rotationNoPitch * dir);
		float projVel = glm::dot(velocity, dir);
		float accelVel = acceleration * dt;

		if (projVel + accelVel > maxVelocity)
		{
			accelVel = maxVelocity - projVel;
		}

		velocity += dir * accelVel;
	}

	if (verticalMovement)
	{
		const float jumpSpeed = SessionSettings::GetSettingValue<float>(ESessionSetting::JUMP_SPEED);
		velocity.y = velocity.y * float32(1 - deltaAction.y) + jumpSpeed * float32(deltaAction.y);
	}
}

void PlayerActionSystem::ComputeAirVelocity(const glm::quat& rotation, const glm::i8vec3& deltaAction, bool walking, float32 dt, glm::vec3& velocity, bool isHoldingFlag)
{
	const float airAccel	= SessionSettings::GetSettingValue<float>(ESessionSetting::AIR_ACCELERATION);
	const float maxVelo		= SessionSettings::GetSettingValue<float>(ESessionSetting::MAX_AIR_VELOCITY);
	ComputeVelocity(rotation, deltaAction, walking, dt, velocity, isHoldingFlag, airAccel, maxVelo);
}

void PlayerActionSystem::ComputeGroundVelocity(const glm::quat& rotation, const glm::i8vec3& deltaAction, bool walking, float32 dt, glm::vec3& velocity, bool isHoldingFlag)
{
	// Apply ground friction
	float speed = glm::length(velocity);
	const float friction = SessionSettings::GetSettingValue<float>(ESessionSetting::GROUND_FRICTION);
	if (speed > glm::epsilon<float>())
	{
 		float drop = speed * friction * dt;
		velocity *= std::max(speed - drop, 0.f) / speed;

		if (glm::length2(velocity) < glm::epsilon<float>())
			velocity *= 0;
	}
	const float maxVelocity = walking ? SessionSettings::GetSettingValue<float>(ESessionSetting::MAX_WALK_VELOCITY) : SessionSettings::GetSettingValue<float>(ESessionSetting::MAX_RUN_VELOCITY);
	const float groundAccel = SessionSettings::GetSettingValue<float>(ESessionSetting::GROUND_ACCELERATION);
	ComputeVelocity(rotation, deltaAction, walking, dt, velocity, isHoldingFlag, groundAccel, maxVelocity);
}

void PlayerActionSystem::SetMouseEnabled(bool isEnabled)
{
	m_MouseEnabled = isEnabled;
}