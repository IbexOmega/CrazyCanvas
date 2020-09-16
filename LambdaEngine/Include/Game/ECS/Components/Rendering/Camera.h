#pragma once

#include "ECS/Component.h"
#include "ECS/Entity.h"
#include "Math/Math.h"

namespace LambdaEngine
{
	struct ViewProjectionMatrices
	{
		DECL_COMPONENT(ViewProjectionMatrices);
		glm::mat4 Projection		= glm::mat4(1.0f);
		glm::mat4 View				= glm::mat4(1.0f);
	};

	struct CameraProperties
	{
		DECL_COMPONENT(CameraProperties);
		glm::vec2 Jitter = glm::vec2(0.0f);
	};

	// Details how to create view and projection matrices
	struct ViewProjectionDesc
	{
		glm::vec3 Position;
		glm::vec3 Direction;
		float FOVDegrees;
		float Width;
		float Height;
		float NearPlane;
		float FarPlane;
	};

	ViewProjectionMatrices CreateViewProjectionMatrices(Entity entity, const ViewProjectionDesc& matricesDesc);
}
