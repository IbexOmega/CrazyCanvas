#include "Game/ECS/Systems/CameraSystem.h"

#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Physics/Transform.h"

#include "ECS/ECSCore.h"

#include <glm/gtx/euler_angles.hpp>
#include "Input/API/Input.h"
#include "Input/API/InputActionSystem.h"
#include "Log/Log.h"
#include "Application/API/CommonApplication.h"
#include "Application/API/Window.h"

#include "Rendering/PhysicsRenderer.h"

namespace LambdaEngine
{
	CameraSystem CameraSystem::s_Instance;

	//The up vector is inverted because of vulkans inverted y-axis
	const glm::vec3 UP_VECTOR = glm::vec3(0.0f, -1.0f, 0.0f);
	const glm::vec3 FORWARD_VECTOR = glm::vec3(0.0f, 0.0f, 1.0f);

	bool CameraSystem::Init()
	{
		TransformComponents transformComponents;
		transformComponents.Position.Permissions	= RW;
		transformComponents.Scale.Permissions		= NDA;
		transformComponents.Rotation.Permissions	= RW;

		// Subscribe on entities with transform and viewProjectionMatrices. They are considered the camera.
		{
			SystemRegistration systemReg = {};
			systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
			{
				{{{RW, CameraComponent::Type()}, {RW, ViewProjectionMatricesComponent::Type()}}, {&transformComponents}, &m_CameraEntities}
			};
			systemReg.SubscriberRegistration.AdditionalDependencies = { {{R, FreeCameraComponent::Type()}} };
			systemReg.Phase = g_LastPhase-1;

			RegisterSystem(systemReg);
		}

		return true;
	}

	void CameraSystem::Tick(Timestamp deltaTime)
	{
		ECSCore* pECSCore = ECSCore::GetInstance();

		ComponentArray<CameraComponent>* pCameraComponents = pECSCore->GetComponentArray<CameraComponent>();
		ComponentArray<FreeCameraComponent>* pFreeCameraComponents = pECSCore->GetComponentArray<FreeCameraComponent>();

		for (Entity entity : m_CameraEntities)
		{
			auto& camComp = pCameraComponents->GetData(entity);
			if (camComp.IsActive)
			{
				auto& viewProjComp = pECSCore->GetComponent<ViewProjectionMatricesComponent>(entity);
				auto& posComp = pECSCore->GetComponent<PositionComponent>(entity);
				auto& rotComp = pECSCore->GetComponent<RotationComponent>(entity);

				TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();
				const uint16 width = window->GetWidth();
				const uint16 height = window->GetHeight();
				camComp.Jitter = glm::vec2((Random::Float32() - 0.5f) / (float)width, (Random::Float32() - 0.5f) / (float)height);

				if(pFreeCameraComponents != nullptr && pFreeCameraComponents->HasComponent(entity))
					HandleInput(deltaTime, entity, posComp, rotComp, pFreeCameraComponents->GetData(entity));

				viewProjComp.View = glm::lookAt(posComp.Position, posComp.Position + GetForward(rotComp.Quaternion), g_DefaultUp);
				camComp.ViewInv = glm::inverse(viewProjComp.View);
				camComp.ProjectionInv = glm::inverse(viewProjComp.Projection);
			}
		}
	}

	void CameraSystem::MainThreadTick(Timestamp)
	{
		if (m_VisbilityChanged)
		{
			CommonApplication::Get()->SetMouseVisibility(!m_MouseEnabled);
			m_VisbilityChanged = false;
		}

		if (m_MouseEnabled)
		{
			CommonApplication::Get()->SetMousePosition(m_NewMousePos.x, m_NewMousePos.y);
		}
	}

	void CameraSystem::HandleInput(Timestamp deltaTime, Entity entity, PositionComponent& posComp, RotationComponent& rotComp, const FreeCameraComponent& freeCamComp)
	{
		float32 dt = float32(deltaTime.AsSeconds());

		glm::vec3 translation = {
				float(InputActionSystem::IsActive("CAM_RIGHT")	 - InputActionSystem::IsActive("CAM_LEFT")),	// X: Right
				float(InputActionSystem::IsActive("CAM_DOWN")    - InputActionSystem::IsActive("CAM_UP")),		// Y: Up
				float(InputActionSystem::IsActive("CAM_FORWARD") - InputActionSystem::IsActive("CAM_BACKWARD"))	// Z: Forward
		};

		const glm::vec3 forward = GetForward(rotComp.Quaternion);
		const glm::vec3 right	= GetRight(rotComp.Quaternion);

		if (glm::length2(translation) > glm::epsilon<float>())
		{
			const float shiftSpeedFactor = InputActionSystem::IsActive("CAM_SPEED_MODIFIER") ? 2.0f : 1.0f;
			translation = glm::normalize(translation) * freeCamComp.SpeedFactor * shiftSpeedFactor * dt;

			posComp.Position += translation.x * right + translation.y * GetUp(rotComp.Quaternion) + translation.z * forward;
		}

		// Rotation from keyboard input. Applied later, after input from mouse has been read as well.
		float addedPitch	= dt * float(InputActionSystem::IsActive("CAM_ROT_UP") - InputActionSystem::IsActive("CAM_ROT_DOWN"));
		float addedYaw		= dt * float(InputActionSystem::IsActive("CAM_ROT_LEFT") - InputActionSystem::IsActive("CAM_ROT_RIGHT"));

		if (InputActionSystem::IsActive("TOGGLE_MOUSE"))
		{
			if (!m_CIsPressed)
			{
				m_MouseEnabled		= !m_MouseEnabled;
				m_VisbilityChanged	= true;
				m_CIsPressed		= true;
			}
		}
		else 
		{
			m_CIsPressed = false;
		}

		if (Input::IsKeyDown(EKey::KEY_T))
		{
			RenderFrustum(entity);
		}

		if (m_MouseEnabled)
		{
			const MouseState& mouseState = Input::GetMouseState();

			TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();
			const uint16 width = window->GetWidth();
			const uint16 height = window->GetHeight();

			const glm::vec2 mouseDelta(mouseState.Position.x - (int)(width * 0.5), mouseState.Position.y - (int)(height * 0.5));
			m_NewMousePos = glm::ivec2((int)(width * 0.5), (int)(height * 0.5));

			if (glm::length(mouseDelta) > glm::epsilon<float>())
			{
				addedYaw	-= freeCamComp.MouseSpeedFactor * (float)mouseDelta.x * dt;
				addedPitch	+= freeCamComp.MouseSpeedFactor * (float)mouseDelta.y * dt;
			}
		}

		const float MAX_PITCH = glm::half_pi<float>() - 0.01f;

		const float currentPitch = glm::clamp(GetPitch(forward) + addedPitch, -MAX_PITCH, MAX_PITCH);
		const float currentYaw = GetYaw(forward) + addedYaw;

		rotComp.Quaternion =
			glm::angleAxis(currentYaw, g_DefaultUp) *		// Yaw
			glm::angleAxis(currentPitch, g_DefaultRight);	// Pitch
	}

	void CameraSystem::RenderFrustum(Entity entity)
	{
		// Returns null if it isn't initilized
		if (PhysicsRenderer::Get())
		{
			// This is a test code - This should probably not be done every tick
			ECSCore* pECSCore = ECSCore::GetInstance();
			auto& posComp = pECSCore->GetComponent<PositionComponent>(entity);
			auto& rotComp = pECSCore->GetComponent<RotationComponent>(entity);
			auto& camComp = pECSCore->GetComponent<CameraComponent>(entity);

			TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();
			const float aspect = (float)window->GetWidth() / (float)window->GetHeight();
			const float tang = tan(glm::radians(camComp.FOV / 2));
			const float nearHeight = camComp.NearPlane * tang;
			const float nearWidth = nearHeight * aspect;
			const float farHeight = camComp.FarPlane * tang;
			const float farWidth = farHeight * aspect;

			const glm::vec3 forward = GetForward(rotComp.Quaternion);
			const glm::vec3 right = GetRight(rotComp.Quaternion);
			const glm::vec3 up = GetUp(rotComp.Quaternion);

			TArray<glm::vec3> points(10);
			const glm::vec3 nearPos = posComp.Position + forward * camComp.NearPlane;
			const glm::vec3 farPos = posComp.Position + forward * camComp.FarPlane;
			// Near TL -> Far TL
			points[0] = nearPos - right * nearWidth + up * nearHeight;
			points[1] = farPos - right * farWidth + up * farHeight;

			// Near BL -> Far BL
			points[2] = nearPos - right * nearWidth - up * nearHeight;
			points[3] = farPos - right * farWidth - up * farWidth;

			// Near TR -> Far TR
			points[4] = nearPos + right * nearWidth + up * nearHeight;
			points[5] = farPos + right * farWidth + up * farHeight;

			// Near BR -> Far BR
			points[6] = nearPos + right * nearWidth - up * nearHeight;
			points[7] = farPos + right * farWidth - up * farHeight;

			// Far TL -> Far TR
			points[8] = farPos - right * farWidth + up * farHeight;
			points[9] = farPos + right * farWidth + up * farHeight;

			if (m_LineGroupEntityIDs.contains(entity))
			{
				m_LineGroupEntityIDs[entity] = PhysicsRenderer::Get()->UpdateLineGroup(m_LineGroupEntityIDs[entity], points, { 0.0f, 1.0f, 0.0f });
			}
			else
			{
				m_LineGroupEntityIDs[entity] = PhysicsRenderer::Get()->UpdateLineGroup(UINT32_MAX, points, { 0.0f, 1.0f, 0.0f });
			}
		}
	}
}
