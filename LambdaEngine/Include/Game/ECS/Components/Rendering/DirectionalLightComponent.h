#pragma once
#include "ECS/Component.h"

namespace LambdaEngine
{
	struct DirectionalLightComponent
	{
		DECL_COMPONENT_WITH_DIRTY_FLAG(DirectionalLightComponent);
		glm::vec4	ColorIntensity	= glm::vec4(1.0f, 1.0f, 1.0f, 15.f);
		glm::quat	Rotation		= glm::identity<glm::quat>();
		float		FrustumWidth	= 45.0f;
		float		FrustumHeight	= 45.0f;
		float		FrustumZNear	= -65.0f;
		float		FrustumZFar		= 15.0f;
	};
}