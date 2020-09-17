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
		//m_Data({}),
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
		m_LastIsDirty(true)
	{
	}

	/*void Camera::Init(const CameraDesc& desc)
	{
		m_Projection		= glm::perspective(glm::radians(desc.FOVDegrees), desc.Width / desc.Height, desc.NearPlane, desc.FarPlane);
		m_ProjectionInv		= glm::inverse(m_Projection);

		m_FOVDegrees		= desc.FOVDegrees;
		m_Width				= desc.Width;
		m_Height			= desc.Height;
		m_NearPlane			= desc.NearPlane;
		m_FarPlane			= desc.FarPlane;

		SetPosition(desc.Position);
		SetDirection(desc.Direction);
		Update();
	}*/

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
		/*m_Data.Jitter = glm::vec2((Random::Float32() - 0.5f) / m_Width, (Random::Float32() - 0.5f) / m_Height);

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
		}*/
	}

	void Camera::HandleInput(Timestamp delta)
	{
		constexpr float CAMERA_MOVEMENT_SPEED = 1.4f;
		constexpr float CAMERA_ROTATION_SPEED = 45.0f;
		constexpr float CAMERA_MOUSE_SPEED = 10.0f;

		glm::vec3 translation(0.0f, 0.0f, 0.0f);

		float32 dt = float32(delta.AsSeconds());

		// Translation
		if (Input::IsKeyDown(EKey::KEY_W) && Input::IsKeyUp(EKey::KEY_S))
		{
			translation.z += dt;
		}
		else if (Input::IsKeyDown(EKey::KEY_S) && Input::IsKeyUp(EKey::KEY_W))
		{
			translation.z -= dt;
		}

		if (Input::IsKeyDown(EKey::KEY_A) && Input::IsKeyUp(EKey::KEY_D))
		{
			translation.x -= dt;
		}
		else if (Input::IsKeyDown(EKey::KEY_D) && Input::IsKeyUp(EKey::KEY_A))
		{
			translation.x += dt;
		}

		if (Input::IsKeyDown(EKey::KEY_Q) && Input::IsKeyUp(EKey::KEY_E))
		{
			translation.y += dt;
		}
		else if (Input::IsKeyDown(EKey::KEY_E) && Input::IsKeyUp(EKey::KEY_Q))
		{
			translation.y -= dt;
		}

		float shiftSpeedFactor = 1.0f;
		if (Input::IsKeyDown(EKey::KEY_LEFT_SHIFT))
		{
			shiftSpeedFactor = 2.f;
		}

		if (glm::length2(translation) > glm::epsilon<float>())
		{
			translation = glm::normalize(translation) * m_SpeedFactor * shiftSpeedFactor;
			Translate(translation);
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

		if (Input::IsKeyDown(EKey::KEY_C))
		{
			if (!m_IsKeyPressed)
				m_Toggle = !m_Toggle;
			m_IsKeyPressed = true;
		}
		else if (Input::IsKeyUp(EKey::KEY_C))
		{
			m_IsKeyPressed = false;
		}

		if (m_Toggle)
		{
			MouseState mouseState = Input::GetMouseState();
			CommonApplication::Get()->SetMouseVisibility(false);

			TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();
			uint16 width = window->GetWidth();
			uint16 height = window->GetHeight();

			glm::vec2 mouseDelta(mouseState.Position.x - (int)(width * 0.5), mouseState.Position.y - (int)(height * 0.5));
			CommonApplication::Get()->SetMousePosition((int)(width * 0.5), (int)(height * 0.5));

			if (glm::length(mouseDelta) > glm::epsilon<float>())
			{
				Rotate(glm::vec3(0.0f, CAMERA_MOUSE_SPEED * (float)mouseDelta.x * delta.AsSeconds(), 0.0f));
				Rotate(glm::vec3(CAMERA_MOUSE_SPEED * (float)mouseDelta.y * delta.AsSeconds(), 0.0f, 0.0f));
			}
		}
		else
		{
			// Should probably not be called every frame
			CommonApplication::Get()->SetMouseVisibility(true);
		}
	}

	void Camera::CalculateVectors()
	{
		m_Right = glm::normalize(glm::cross(m_Forward, UP_VECTOR));
		m_Up	= glm::normalize(glm::cross(m_Right, m_Forward));
	}
}
