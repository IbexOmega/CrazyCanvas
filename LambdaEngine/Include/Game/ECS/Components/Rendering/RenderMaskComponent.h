#pragma once
#include "ECS/Component.h"

namespace LambdaEngine
{
	/*
	* RenderMaskComponent
	*/
	struct RenderMaskComponent
	{
		DECL_COMPONENT(RenderMaskComponent);
		uint32 Mask = 0x0;
	};
}