#pragma once


#include "ECS/Component.h"

namespace LambdaEngine
{
	struct DirectionalLightComponent
	{
		DECL_COMPONENT_WITH_DIRTY_FLAG(DirectionalLightComponent);
		glm::vec4	ColorIntensity	= glm::vec4(1.0f, 1.0f, 1.0f, 15.f);
		float		FrustumWidth	= 20.0f;
		float		FrustumHeight	= 20.0f;
		float		FrustumZNear	= -80.0f;
		float		FrustumZFar		= 10.0f;
	};
}