#include "Game/Camera.h"

#include <glm/gtx/euler_angles.hpp>
#include "Input/API/Input.h"
#include "Log/Log.h"
#include "Application/API/CommonApplication.h"
#include "Application/API/Window.h"

//The up vector is inverted because of vulkans inverted y-axis
const glm::vec3 UP_VECTOR = glm::vec3(0.0f, -1.0f, 0.0f);
const glm::vec3 FORWARD_VECTOR = glm::vec3(0.0f, 0.0f, 1.0f);

namespace LambdaEngine
{
	Camera::Camera() :
		m_Data({}),
		m_Projection(1.0f),
		m_ProjectionInv(1.0f),
		m_View(1.0f),
		m_ViewInv(1.0f),
		m_Position(0.0f),
		m_Rotation(0.0f),
		m_Forward(0.0f, 0.0f, 1.0f),
		m_Right(0.0f),
		m_Up(0.0f),
		m_IsDirty(true),
		m_LastIsDirty(true),
		m_CommonApplication(nullptr)
	{
	}

	void Camera::Init(CommonApplication* commonApplication, const CameraDesc& desc)
	{
		m_CommonApplication = commonApplication;
		m_Projection		= glm::perspective(glm::radians(desc.FOVDegrees), desc.Width / desc.Height, desc.NearPlane, desc.FarPlane);
		m_ProjectionInv		= glm::inverse(m_Projection);

		SetPosition(desc.Position);
		SetDirection(desc.Direction);
		Update();
	}

	void Camera::SetDirection(const glm::vec3& direction)
	{
		m_Forward = glm::normalize(direction);
		CalculateVectors();

		m_IsDirty		= true;
		m_LastIsDirty	= true;
	}

	void Camera::SetPosition(const glm::vec3& position)
	{
		m_Position		= position;
		m_IsDirty		= true;
		m_LastIsDirty	= true;
	}

	void Camera::SetRotation(const glm::vec3& rotation)
	{
		m_Rotation = rotation;
		m_Rotation.x = glm::max(glm::min(m_Rotation.x, 89.0f), -89.0f);

		glm::mat3 rotationMat = glm::eulerAngleYXZ(glm::radians(m_Rotation.y + 90.0f), glm::radians(m_Rotation.x), glm::radians(m_Rotation.z));
		m_Forward = glm::normalize(rotationMat * FORWARD_VECTOR);

		CalculateVectors();

		m_IsDirty		= true;
		m_LastIsDirty	= true;
	}

	void Camera::Rotate(const glm::vec3& rotation)
	{
		SetRotation(m_Rotation + rotation);
	}

	void Camera::Translate(const glm::vec3& translation)
	{
		m_Position		+= (m_Right * translation.x) + (m_Up * translation.y) + (m_Forward * translation.z);
		m_IsDirty		= true;
		m_LastIsDirty	= true;
	}

	void Camera::Update()
	{
		if (m_IsDirty)
		{
			//Update view
			m_View = glm::lookAt(m_Position, m_Position + m_Forward, m_Up);
			m_ViewInv = glm::inverse(m_View);

			m_Data.PrevProjection	= m_Data.Projection;
			m_Data.PrevView			= m_Data.View;
			m_Data.Projection		= m_Projection;
			m_Data.View				= m_View;
			m_Data.ViewInv			= m_ViewInv;
			m_Data.ProjectionInv	= m_ProjectionInv;
			m_Data.Position			= glm::vec4(m_Position, 1.0f);
			m_Data.Right			= glm::vec4(m_Right, 0.0f);
			m_Data.Up				= glm::vec4(m_Up, 0.0f);

			m_IsDirty = false;
		}
		else if (m_LastIsDirty)
		{
			m_Data.PrevProjection	= m_Data.Projection;
			m_Data.PrevView			= m_Data.View;

			m_LastIsDirty = false;
		}
	}

	void Camera::HandleInput(Timestamp delta)
	{
		constexpr float CAMERA_MOVEMENT_SPEED = 1.4f;
		constexpr float CAMERA_ROTATION_SPEED = 45.0f;
		constexpr float CAMERA_MOUSE_SPEED = 10.0f;

		// Translation
		if (Input::IsKeyDown(EKey::KEY_W) && Input::IsKeyUp(EKey::KEY_S))
		{
			Translate(glm::vec3(0.0f, 0.0f, CAMERA_MOVEMENT_SPEED * delta.AsSeconds()));
		}
		else if (Input::IsKeyDown(EKey::KEY_S) && Input::IsKeyUp(EKey::KEY_W))
		{
			Translate(glm::vec3(0.0f, 0.0f, -CAMERA_MOVEMENT_SPEED * delta.AsSeconds()));
		}

		if (Input::IsKeyDown(EKey::KEY_A) && Input::IsKeyUp(EKey::KEY_D))
		{
			Translate(glm::vec3(-CAMERA_MOVEMENT_SPEED * delta.AsSeconds(), 0.0f, 0.0f));
		}
		else if (Input::IsKeyDown(EKey::KEY_D) && Input::IsKeyUp(EKey::KEY_A))
		{
			Translate(glm::vec3(CAMERA_MOVEMENT_SPEED * delta.AsSeconds(), 0.0f, 0.0f));
		}

		if (Input::IsKeyDown(EKey::KEY_Q) && Input::IsKeyUp(EKey::KEY_E))
		{
			Translate(glm::vec3(0.0f, CAMERA_MOVEMENT_SPEED * delta.AsSeconds(), 0.0f));
		}
		else if (Input::IsKeyDown(EKey::KEY_E) && Input::IsKeyUp(EKey::KEY_Q))
		{
			Translate(glm::vec3(0.0f, -CAMERA_MOVEMENT_SPEED * delta.AsSeconds(), 0.0f));
		}

		// Rotation
		if (Input::IsKeyDown(EKey::KEY_UP) && Input::IsKeyUp(EKey::KEY_DOWN))
		{
			Rotate(glm::vec3(-CAMERA_ROTATION_SPEED * delta.AsSeconds(), 0.0f, 0.0f));
		}
		else if (Input::IsKeyDown(EKey::KEY_DOWN) && Input::IsKeyUp(EKey::KEY_UP))
		{
			Rotate(glm::vec3(CAMERA_ROTATION_SPEED * delta.AsSeconds(), 0.0f, 0.0f));
		}

		if (Input::IsKeyDown(EKey::KEY_LEFT) && Input::IsKeyUp(EKey::KEY_RIGHT))
		{
			Rotate(glm::vec3(0.0f, -CAMERA_ROTATION_SPEED * delta.AsSeconds(), 0.0f));
		}
		else if (Input::IsKeyDown(EKey::KEY_RIGHT) && Input::IsKeyUp(EKey::KEY_LEFT))
		{
			Rotate(glm::vec3(0.0f, CAMERA_ROTATION_SPEED * delta.AsSeconds(), 0.0f));
		}

		MouseState mouseState = Input::GetMouseState();
		if (mouseState.IsButtonPressed(EMouseButton::MOUSE_BUTTON_RIGHT))
		{
			m_CommonApplication->SetMouseVisibility(false);

			uint16 width	= m_CommonApplication->GetActiveWindow()->GetWidth();
			uint16 height	= m_CommonApplication->GetActiveWindow()->GetHeight();

			glm::vec2 mouseDelta(mouseState.x - (int)(width * 0.5), mouseState.y - (int)(height * 0.5));
			m_CommonApplication->SetMousePosition((int)(width * 0.5), (int)(height * 0.5));

			if (glm::length(mouseDelta) > glm::epsilon<float>())
			{
				Rotate(glm::vec3(0.0f, CAMERA_MOUSE_SPEED * mouseDelta.x * delta.AsSeconds(), 0.0f));
				Rotate(glm::vec3(CAMERA_MOUSE_SPEED * mouseDelta.y * delta.AsSeconds(), 0.0f, 0.0f));
			}
		}
		else
		{
			// Should probably not be called every frame
			m_CommonApplication->SetMouseVisibility(true);
		}
	}

	void Camera::CalculateVectors()
	{
		m_Right = glm::normalize(glm::cross(m_Forward, UP_VECTOR));
		m_Up	= glm::normalize(glm::cross(m_Right, m_Forward));
	}
}
