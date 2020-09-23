#pragma once


#include "ECS/Component.h"

namespace LambdaEngine
{
	struct DirectionalLightComponent
	{
		DECL_COMPONENT(DirectionalLightComponent);
		glm::vec4	ColorIntensity;
	};
}