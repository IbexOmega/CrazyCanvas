#include "Game/ECS/Systems/CameraSystem.h"

#include "Application/API/CommonApplication.h"

#include "ECS/ECSCore.h"

#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Physics/Transform.h"

#include "Input/API/Input.h"
#include "Input/API/InputActionSystem.h"
#include "Log/Log.h"

#include "Rendering/LineRenderer.h"

namespace LambdaEngine
{
	CameraSystem CameraSystem::s_Instance;

	bool CameraSystem::Init()
	{
		// Subscribe on entities with transform and viewProjectionMatrices. They are considered the camera.
		{
			SystemRegistration systemReg = {};
			systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
			{
				{
					{
						{R, CameraComponent::Type()}, 
						{NDA, ViewProjectionMatricesComponent::Type()}, 
						{RW, VelocityComponent::Type()},
						{NDA, PositionComponent::Type()}, 
						{RW, RotationComponent::Type()},
					},
					&m_CameraEntities
				},
				{
					{
						{R, CameraComponent::Type()}, 
						{NDA, ViewProjectionMatricesComponent::Type()}, 
						{R, ParentComponent::Type()},
						{R, OffsetComponent::Type()},
						{RW, PositionComponent::Type()}, 
						{RW, RotationComponent::Type()},
					},
					&m_AttachedCameraEntities
				}
			};
			systemReg.SubscriberRegistration.AdditionalDependencies = { {{R, FreeCameraComponent::Type()}, {R, FPSControllerComponent::Type()}} };
			systemReg.Phase = 0;

			RegisterSystem(systemReg);
		}

		return true;
	}

	void CameraSystem::Tick(Timestamp deltaTime)
	{
		const float32 dt = (float32)deltaTime.AsSeconds();
		ECSCore* pECSCore = ECSCore::GetInstance();

		const ComponentArray<CameraComponent>*			pCameraComponents		= pECSCore->GetComponentArray<CameraComponent>();
		const ComponentArray<FreeCameraComponent>*		pFreeCameraComponents	= pECSCore->GetComponentArray<FreeCameraComponent>();
		const ComponentArray<FPSControllerComponent>*	pFPSCameraComponents	= pECSCore->GetComponentArray<FPSControllerComponent>();
		const ComponentArray<ParentComponent>*			pParentComponents		= pECSCore->GetComponentArray<ParentComponent>();
		const ComponentArray<OffsetComponent>*			pOffsetComponents		= pECSCore->GetComponentArray<OffsetComponent>();
		ComponentArray<PositionComponent>*				pPositionComponents		= pECSCore->GetComponentArray<PositionComponent>();
		ComponentArray<RotationComponent>*				pRotationComponents		= pECSCore->GetComponentArray<RotationComponent>();
		ComponentArray<VelocityComponent>*				pVelocityComponents		= pECSCore->GetComponentArray<VelocityComponent>();

		for (Entity entity : m_AttachedCameraEntities)
		{
			const ParentComponent&		parentComp			= pParentComponents->GetData(entity);

			if (parentComp.Attached)
			{
				const PositionComponent&	parentPositionComp	= pPositionComponents->GetData(parentComp.Parent);
				const RotationComponent&	parentRotationComp	= pRotationComponents->GetData(parentComp.Parent);
				const OffsetComponent&		cameraOffsetComp	= pOffsetComponents->GetData(entity);
				PositionComponent&			cameraPositionComp	= pPositionComponents->GetData(entity);
				RotationComponent&			cameraRotationComp	= pRotationComponents->GetData(entity);
			
				cameraPositionComp.Position		= parentPositionComp.Position + cameraOffsetComp.Offset;
				cameraRotationComp.Quaternion	= parentRotationComp.Quaternion;
			}
		}

		for (Entity entity : m_CameraEntities)
		{
			const auto& camComp = pCameraComponents->GetData(entity);
			if (camComp.IsActive)
			{
				auto& rotationComp	= pRotationComponents->GetData(entity);
				auto& velocityComp	= pVelocityComponents->GetData(entity);

				if(pFreeCameraComponents != nullptr && pFreeCameraComponents->HasComponent(entity))
				{
					MoveFreeCamera(dt, velocityComp, rotationComp, pFreeCameraComponents->GetData(entity));
				}
				else if (pFPSCameraComponents && pFPSCameraComponents->HasComponent(entity))
				{
					MoveFPSCamera(dt, velocityComp, rotationComp, pFPSCameraComponents->GetData(entity));
				}

				#ifdef LAMBDA_DEBUG
					if (Input::IsKeyDown(EKey::KEY_T))
					{
						RenderFrustum(entity, pPositionComponents->GetData(entity), rotationComp);
					}
				#endif // LAMBDA_DEBUG
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

	void CameraSystem::MoveFreeCamera(float32 dt, VelocityComponent& velocityComp, RotationComponent& rotationComp, const FreeCameraComponent& freeCamComp)
	{
		glm::vec3& velocity = velocityComp.Velocity;
		velocity = {
			float(InputActionSystem::IsActive("CAM_RIGHT")		- InputActionSystem::IsActive("CAM_LEFT")),		// X: Right
			float(InputActionSystem::IsActive("CAM_UP")			- InputActionSystem::IsActive("CAM_DOWN")),		// Y: Up
			float(InputActionSystem::IsActive("CAM_FORWARD")	- InputActionSystem::IsActive("CAM_BACKWARD"))	// Z: Forward
		};

		const glm::vec3 forward = GetForward(rotationComp.Quaternion);

		if (glm::length2(velocity) > glm::epsilon<float>())
		{
			const glm::vec3 right = GetRight(rotationComp.Quaternion);
			const float shiftSpeedFactor = InputActionSystem::IsActive("CAM_SPEED_MODIFIER") ? 2.0f : 1.0f;
			velocity = glm::normalize(velocity) * freeCamComp.SpeedFactor * shiftSpeedFactor;

			velocity = velocity.x * right + velocity.y * GetUp(rotationComp.Quaternion) + velocity.z * forward;
		}

		RotateCamera(dt, freeCamComp.MouseSpeedFactor, forward, rotationComp.Quaternion);
	}

	void CameraSystem::MoveFPSCamera(float32 dt, VelocityComponent& velocityComp, RotationComponent& rotationComp, const FPSControllerComponent& FPSComp)
	{
		// First calculate translation relative to the character's rotation (i.e. right, up, forward).
		// Then convert the translation be relative to the world axes.
		glm::vec3& velocity = velocityComp.Velocity;

		glm::vec2 horizontalVelocity = {
			float(InputActionSystem::IsActive("CAM_RIGHT") - InputActionSystem::IsActive("CAM_LEFT")),			// X: Right
			float(InputActionSystem::IsActive("CAM_FORWARD")	- InputActionSystem::IsActive("CAM_BACKWARD"))	// Y: Forward
		};

		if (glm::length2(horizontalVelocity) > glm::epsilon<float>())
		{
			const int8 isSprinting = InputActionSystem::IsActive("CAM_SPEED_MODIFIER");
			const float32 sprintFactor = std::max(1.0f, FPSComp.SprintSpeedFactor * isSprinting);
			horizontalVelocity = glm::normalize(horizontalVelocity) * FPSComp.SpeedFactor * sprintFactor;
		}

		velocity.x = horizontalVelocity.x;
		velocity.y -= GRAVITATIONAL_ACCELERATION * dt;
		velocity.z = horizontalVelocity.y;

		const glm::vec3 forward	= GetForward(rotationComp.Quaternion);
		const glm::vec3 right	= GetRight(rotationComp.Quaternion);

		const glm::vec3 forwardHorizontal	= glm::normalize(glm::vec3(forward.x, 0.0f, forward.z));
		const glm::vec3 rightHorizontal		= glm::normalize(glm::vec3(right.x, 0.0f, right.z));

		velocity = velocity.x * rightHorizontal + velocity.y * g_DefaultUp + velocity.z * forwardHorizontal;

		RotateCamera(dt, FPSComp.MouseSpeedFactor, forward, rotationComp.Quaternion);
	}

	void CameraSystem::RotateCamera(float32 dt, float32 mouseSpeedFactor, const glm::vec3& forward, glm::quat& rotation)
	{
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
				addedYaw	-= mouseSpeedFactor * (float)mouseDelta.x * dt;
				addedPitch	-= mouseSpeedFactor * (float)mouseDelta.y * dt;
			}
		}

		const float MAX_PITCH = glm::half_pi<float>() - 0.01f;

		const float currentPitch = glm::clamp(GetPitch(forward) + addedPitch, -MAX_PITCH, MAX_PITCH);
		const float currentYaw = GetYaw(forward) + addedYaw;

		rotation =
			glm::angleAxis(currentYaw, g_DefaultUp) *		// Yaw
			glm::angleAxis(currentPitch, g_DefaultRight);	// Pitch
	}

	void CameraSystem::RenderFrustum(Entity entity, const PositionComponent& positionComp, const RotationComponent& rotationComp)
	{
		LineRenderer* pLineRenderer = LineRenderer::Get();

		if (pLineRenderer != nullptr)
		{
			// This is a test code - This should probably not be done every tick
			ECSCore* pECSCore = ECSCore::GetInstance();
			auto& posComp = pECSCore->GetComponent<PositionComponent>(entity);
			auto& rotationComp = pECSCore->GetComponent<RotationComponent>(entity);
			auto& camComp = pECSCore->GetComponent<CameraComponent>(entity);

			TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();
			const float aspect = (float)window->GetWidth() / (float)window->GetHeight();
			const float tang = tan(glm::radians(camComp.FOV / 2));
			const float nearHeight = camComp.NearPlane * tang;
			const float nearWidth = nearHeight * aspect;
			const float farHeight = camComp.FarPlane * tang;
			const float farWidth = farHeight * aspect;

			const glm::vec3 forward = GetForward(rotationComp.Quaternion);
			const glm::vec3 right = GetRight(rotationComp.Quaternion);
			const glm::vec3 up = GetUp(rotationComp.Quaternion);

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
				m_LineGroupEntityIDs[entity] = pLineRenderer->UpdateLineGroup(m_LineGroupEntityIDs[entity], points, { 0.0f, 1.0f, 0.0f });
			}
			else
			{
				m_LineGroupEntityIDs[entity] = pLineRenderer->UpdateLineGroup(UINT32_MAX, points, { 0.0f, 1.0f, 0.0f });
			}
		}
	}
}
