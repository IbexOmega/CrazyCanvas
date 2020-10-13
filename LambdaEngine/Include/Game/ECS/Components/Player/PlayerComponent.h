#pragma once

#include "ECS/Component.h"

namespace LambdaEngine
{
	struct PlayerComponent
	{
		DECL_COMPONENT(PlayerComponent);
		float WalkSpeedFactor	= 1.4f;
		float SprintSpeedFactor = 1.6f;
		bool IsLocal;
	};
}