#pragma once

#include "ECS/Component.h"

namespace LambdaEngine
{
	struct PositionComponent
	{
		glm::vec3 Pos;
	};

	struct ScaleComponent
	{
		glm::vec3 Scale;
	};

	struct RotationComponent
	{
		glm::quat Quaternion;
	};

	struct NameComponent
	{
		char Name[64];
	};
}