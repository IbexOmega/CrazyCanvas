#pragma once


#include "ECS/Component.h"

namespace LambdaEngine
{
	struct DirectionalLightComponent
	{
		DECL_COMPONENT_WITH_DIRTY_FLAG(DirectionalLightComponent);
		glm::vec4	ColorIntensity	= glm::vec4(1.0f, 1.0f, 1.0f, 25.f);
		float		frustumWidth	= 10.0f;
		float		frustumHeight	= 10.0f;
		float		frustumZNear	= -5.0f;
		float		frustumZFar		= 10.0f;
	};
}