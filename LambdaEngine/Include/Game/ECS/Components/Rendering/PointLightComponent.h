#pragma once

#include "ECS/Component.h"

namespace LambdaEngine
{
	struct PointLightComponent
	{
		DECL_COMPONENT(PointLightComponent);
		glm::vec4	ColorIntensity;
	};
}