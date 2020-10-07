#pragma once

#include "ECS/Component.h"
#include "Math/Math.h"
#include "Time/API/Timestamp.h"
#include "Engine/EngineLoop.h"

namespace LambdaEngine
{
	struct NetworkPositionComponent
	{
		DECL_COMPONENT(NetworkPositionComponent);
		glm::vec3 Position			= glm::vec3(0.0f);
		glm::vec3 PositionLast		= glm::vec3(0.0f);
		Timestamp TimestampStart	= 0;
		Timestamp Duration			= EngineLoop::GetFixedTimestep();
	};
}
