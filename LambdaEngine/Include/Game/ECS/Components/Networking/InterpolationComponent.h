#pragma once

#include "ECS/Component.h"
#include "Time/API/Timestamp.h"

namespace LambdaEngine
{
	struct InterpolationComponent
	{
		DECL_COMPONENT(InterpolationComponent);

		glm::vec3 StartPosition		= glm::vec3(0.0f);
		glm::vec3 EndPosition		= glm::vec3(0.0f);
		Timestamp StartTimestamp	= 0;
		Timestamp Duration			= 0;
	};
}