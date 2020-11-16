#pragma once

#include "ECS/Component.h"

namespace LambdaEngine
{
	struct RayTracedComponent
	{
		DECL_COMPONENT(RayTracedComponent);
		uint8 HitMask	= 0xFF;
	};
}