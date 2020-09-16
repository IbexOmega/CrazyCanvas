#pragma once

#include "ECS/Component.h"

namespace LambdaEngine
{
	struct TestPositionComponent
	{
		DECL_COMPONENT(TestPositionComponent);
		glm::vec3 Pos;
	};

	struct TestScaleComponent
	{
		DECL_COMPONENT(TestScaleComponent);
		glm::vec3 Scale;
	};

	struct TestRotationComponent
	{
		DECL_COMPONENT(TestRotationComponent);
		glm::quat Quaternion;
	};

	struct TestNameComponent
	{
		DECL_COMPONENT(TestNameComponent);
		char Name[64];
	};
}