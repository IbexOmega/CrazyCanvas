#pragma once

#include "ECS/Component.h"
#include "ECS/Entity.h"
#include "Math/Math.h"

namespace LambdaEngine
{
	struct FreeCameraComponent
	{
		DECL_COMPONENT(FreeCameraComponent);
		float SpeedFactor = 1.4f;
		float MouseSpeedFactor = 1.f;
	};

	struct CameraComponent
	{
		DECL_COMPONENT(CameraComponent);
		glm::vec2 Jitter = glm::vec2(0.f);
		glm::mat4 ProjectionInv = glm::mat4(1.f);
		glm::mat4 ViewInv = glm::mat4(1.f);
		bool IsActive = true;
	};

	struct ViewProjectionMatricesComponent
	{
		DECL_COMPONENT(ViewProjectionMatricesComponent);
		glm::mat4 Projection	= glm::mat4(1.0f);
		glm::mat4 View			= glm::mat4(1.0f);
	};

	struct CameraDesc
	{
		glm::vec3 Position = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 Direction = glm::vec3(0.0f, 0.0f, 1.0f);
		float FOVDegrees = 45.0f;
		float Width = 1280.0f;
		float Height = 720.0f;
		float NearPlane = 0.0001f;
		float FarPlane = 50.0f;
	};

	void CreateFreeCameraEntity(const CameraDesc& cameraDesc);
}
