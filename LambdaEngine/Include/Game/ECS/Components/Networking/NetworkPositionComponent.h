#pragma once

#include "ECS/Component.h"
#include "Math/Math.h"
#include "Time/API/Timestamp.h"
#include "Engine/EngineLoop.h"

namespace LambdaEngine
{
	struct NetworkPositionComponent
	{
		DECL_COMPONENT_WITH_DIRTY_FLAG(NetworkPositionComponent);
		glm::vec3 Position			= glm::vec3(0.0f);
		glm::vec3 PositionLast		= glm::vec3(0.0f);
		Timestamp TimestampStart	= EngineLoop::GetTimeSinceStart();
		Timestamp Duration			= EngineLoop::GetFixedTimestep();
		mutable uint64 Frame = 0;
	};
}
