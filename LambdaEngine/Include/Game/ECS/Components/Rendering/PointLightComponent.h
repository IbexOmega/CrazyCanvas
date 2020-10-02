#pragma once

#include "ECS/Component.h"

namespace LambdaEngine
{
	struct PointLightComponent
	{
		DECL_COMPONENT_WITH_DIRTY_FLAG(PointLightComponent);
		glm::vec4	ColorIntensity	= glm::vec4(1.0);
		float		NearPlane		= 0.1f;
		float		FarPlane		= 25.0f;
	};
}