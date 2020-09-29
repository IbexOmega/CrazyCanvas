#pragma once

#include "ECS/Component.h"

namespace LambdaEngine
{
	struct PointLightComponent
	{
		DECL_COMPONENT(PointLightComponent);
		glm::vec4	ColorIntensity		= glm::vec4(1.0);
		float		NearPlane			= 0.1f;
		float		FarPlane			= 10.0f;
		bool		Dirty = true;
	};
}