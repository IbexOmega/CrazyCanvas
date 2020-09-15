#pragma once

#include "ECS/Component.h"

namespace LambdaEngine
{
	struct PositionComponent : public IComponent
	{
		glm::vec3 Pos;
	};

	struct ScaleComponent : public IComponent
	{
		glm::vec3 Scale;
	};

	struct RotationComponent : public IComponent
	{
		glm::quat Quaternion;
	};
}