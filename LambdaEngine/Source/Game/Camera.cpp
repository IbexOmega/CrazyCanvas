#include "Game/Camera.h"

#include <glm/gtx/euler_angles.hpp>

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
		m_LastIsDirty(true)
	{
	}

	void Camera::Init(const CameraDesc& desc)
	{
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

			m_Data.LastProjection	= m_Data.Projection;
			m_Data.LastView			= m_Data.View;
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
			m_Data.LastProjection	= m_Data.Projection;
			m_Data.LastView			= m_Data.View;

			m_LastIsDirty = false;
		}
	}

	void Camera::CalculateVectors()
	{
		m_Right = glm::normalize(glm::cross(m_Forward, UP_VECTOR));
		m_Up	= glm::normalize(glm::cross(m_Right, m_Forward));
	}
}
