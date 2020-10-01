#pragma once

#include "ECS/Component.h"

namespace LambdaEngine
{
	struct ControllableComponent
	{
		DECL_COMPONENT(ControllableComponent);
		bool IsActive = false;
	};
}