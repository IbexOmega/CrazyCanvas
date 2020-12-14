#pragma once

#include "ECS/Component.h"

namespace LambdaEngine
{
	enum FRayTracingHitMask : uint8
	{
		OCCLUDER			= FLAG(0),
		PARTICLE_COLLIDABLE = FLAG(1),
		ALL					= UINT8_MAX
	};

	struct RayTracedComponent
	{
		DECL_COMPONENT(RayTracedComponent);
		uint8 HitMask	= 0xFF;
	};
}