#include "Game/ECS/Rendering/CameraController.h"

#include "Application/API/CommonApplication.h"
#include "Application/API/Events/EventHandler.h"
#include "Application/API/Events/EventQueue.h"
#include "Game/ECS/Rendering/ComponentGroups.h"
#include "Input/API/Input.h"
#include "Math/Math.h"
#include "Math/Random.h"

namespace LambdaEngine
{
	CameraController::CameraController()
		:m_pCameraHandler(nullptr),
		m_pTransformHandler(nullptr),
		m_WindowSize(0),
		m_MouseEnabled(false),
		m_CIsPressed(Input::IsKeyDown(EKey::KEY_C))
	{
		CameraComponents cameraComponents;
		cameraComponents.Position.Permissions				= RW;
		cameraComponents.Rotation.Permissions				= RW;
		cameraComponents.ViewProjectionMatrices.Permissions	= RW;
		cameraComponents.CameraProperties.Permissions		= RW;

		SystemRegistration sysReg = {};
		sysReg.SubscriberRegistration.EntitySubscriptionRegistrations =
		{
			{{&cameraComponents}, &m_Cameras},
		};
		sysReg.Phase = g_LastPhase - 1u;

		EnqueueRegistration(sysReg);

		EventQueue::RegisterEventHandler<KeyPressedEvent>(EventHandler(this, &CameraController::OnScreenResize));
	}

	CameraController::~CameraController()
	{
		EventQueue::UnregisterEventHandler<KeyPressedEvent>(EventHandler(this, &CameraController::OnScreenResize));
	}

	bool CameraController::InitSystem()
	{
		m_pCameraHandler = reinterpret_cast<CameraHandler*>(GetComponentHandler(TID(CameraHandler)));
		return m_pCameraHandler;
	}

	void CameraController::Tick(float dt)
	{
		for (Entity camera : m_Cameras.GetIDs())
		{
			CameraProperties& cameraProperties = m_pCameraHandler->GetCameraProperties(camera);
			cameraProperties.Jitter = glm::vec2((Random::Float32() - 0.5f) / m_WindowSize.x, (Random::Float32() - 0.5f) / m_WindowSize.y);

			constexpr float CAMERA_MOVEMENT_SPEED = 0.05f;
			constexpr float CAMERA_ROTATION_SPEED = 45.0f;
			constexpr float CAMERA_MOUSE_SPEED = 10.0f;

			glm::vec3 translation = {
				dt * float(Input::IsKeyDown(EKey::KEY_D) - Input::IsKeyDown(EKey::KEY_A)),	// X: Right
				dt * float(Input::IsKeyDown(EKey::KEY_Q) - Input::IsKeyDown(EKey::KEY_E)),	// Y: Up
				dt * float(Input::IsKeyDown(EKey::KEY_W) - Input::IsKeyDown(EKey::KEY_S))	// Z: Forward
			};

			glm::vec3& position = m_pTransformHandler->GetPosition(camera);
			glm::quat& rotation = m_pTransformHandler->GetRotation(camera);
			const glm::vec3 forward = TransformHandler::GetForward(rotation);
			const glm::vec3 right = TransformHandler::GetRight(rotation);

			if (glm::length2(translation) > glm::epsilon<float>())
			{
				const float shiftSpeedFactor = Input::IsKeyDown(EKey::KEY_LEFT_SHIFT) ? 2.0f : 1.0f;
				translation = glm::normalize(translation) * CAMERA_MOVEMENT_SPEED * shiftSpeedFactor;

				position += glm::vec3(
					translation.x * right,
					translation.y * TransformHandler::GetUp(rotation),
					translation.z * forward
				);
			}

			// Rotation from keyboard input. Applied later, after input from mouse has been read as well.
			float addedPitch = dt * float(Input::IsKeyDown(EKey::KEY_UP) - Input::IsKeyDown(EKey::KEY_DOWN));
			float addedYaw = dt * float(Input::IsKeyDown(EKey::KEY_LEFT) - Input::IsKeyDown(EKey::KEY_RIGHT));

			if (Input::IsKeyDown(EKey::KEY_C))
			{
				if (!m_CIsPressed)
				{
					m_MouseEnabled = !m_MouseEnabled;
					CommonApplication::Get()->SetMouseVisibility(!m_MouseEnabled);
				}
				m_CIsPressed = true;
			}
			else if (Input::IsKeyUp(EKey::KEY_C))
			{
				// Should probably not be called every frame
				m_CIsPressed = false;
			}

			if (m_MouseEnabled)
			{
				const MouseState& mouseState = Input::GetMouseState();

				TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();
				uint16 width = window->GetWidth();
				uint16 height = window->GetHeight();

				glm::vec2 mouseDelta(mouseState.Position.x - (int)(width * 0.5), mouseState.Position.y - (int)(height * 0.5));
				CommonApplication::Get()->SetMousePosition((int)(width * 0.5), (int)(height * 0.5));

				addedYaw += CAMERA_MOUSE_SPEED * (float)mouseDelta.x * dt;
				addedPitch += CAMERA_MOUSE_SPEED * (float)mouseDelta.y * dt;
			}

			const float MAX_PITCH = glm::half_pi<float>() - 0.01f;
			float currentPitch = TransformHandler::GetPitch(forward);
			currentPitch = glm::clamp(currentPitch + addedPitch, -MAX_PITCH, MAX_PITCH);

			rotation = glm::rotate(rotation, addedPitch, right);
			rotation = glm::rotate(rotation, addedPitch, g_DefaultUp);

			ViewProjectionMatrices& viewProjectionMatrices = m_pCameraHandler->GetViewProjectionMatrices(camera);
			viewProjectionMatrices.PrevView = viewProjectionMatrices.View;
			viewProjectionMatrices.View = glm::lookAt(position, position + TransformHandler::GetForward(rotation), g_DefaultUp);
		}
	}

	bool CameraController::OnScreenResize(const WindowResizedEvent& event)
	{
		m_WindowSize = {
			event.Width,
			event.Height
		};
		return true;
	}
}
