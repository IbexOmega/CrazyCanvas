#pragma once

#include "ECS/Component.h"

namespace LambdaEngine
{
	struct MaskComponent
	{
		DECL_DRAW_ARG_COMPONENT(MaskComponent);
		uint32 Mask = UINT32_MAX;
	};
}