#pragma once

#include "ECS/Component.h"
#include "ECS/Entity.h"
#include "Game/ECS/Components/Physics/Transform.h"
#include "Math/Math.h"

namespace LambdaEngine
{
	struct FreeCameraComponent
	{
		DECL_COMPONENT(FreeCameraComponent);
		float SpeedFactor = 1.4f;
		float MouseSpeedFactor = 0.35f;
	};

	struct FPSControllerComponent : FreeCameraComponent
	{
		DECL_COMPONENT(FPSControllerComponent);
		float SprintSpeedFactor	= 1.6f;
	};

	struct CameraComponent
	{
		DECL_COMPONENT(CameraComponent);
		glm::vec2 Jitter 		= glm::vec2(0.f);
		glm::mat4 ProjectionInv = glm::mat4(1.f);
		glm::mat4 ViewInv 		= glm::mat4(1.f);
		float NearPlane 		= 0.0001f;
		float FarPlane			= 50.0f;
		float FOV				= 45.0f;
		bool IsActive 			= true;
	};

	struct ViewProjectionMatricesComponent
	{
		DECL_COMPONENT(ViewProjectionMatricesComponent);
		glm::mat4 Projection	= glm::mat4(1.0f);
		glm::mat4 View			= glm::mat4(1.0f);
	};

	struct CameraDesc
	{
		glm::vec3 Position 	= 2.0f * g_DefaultUp;
		glm::vec3 Direction = g_DefaultForward;
		float FOVDegrees 	= 45.0f;
		float Width 		= 1280.0f;
		float Height 		= 720.0f;
		float NearPlane 	= 0.0001f;
		float FarPlane 		= 50.0f;
	};

	Entity CreateFreeCameraEntity(const CameraDesc& cameraDesc);
	Entity CreateFPSCameraEntity(const CameraDesc& cameraDesc);
	Entity CreateCameraTrackEntity(const CameraDesc& cameraDesc, const TArray<glm::vec3>& track);

	static Entity CreateCameraEntity(const CameraDesc& cameraDesc);
}
