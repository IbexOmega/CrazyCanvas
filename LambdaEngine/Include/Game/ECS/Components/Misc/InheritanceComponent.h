#pragma once

#include "ECS/Component.h"
#include "ECS/Entity.h"

namespace LambdaEngine
{
	constexpr const uint32 MAX_CHILD_COMPONENTS = 8;

	struct ParentComponent
	{
		DECL_COMPONENT(ParentComponent);
		Entity	Parent;
		bool	Attached;
	};

	struct ChildComponent
	{
		DECL_COMPONENT(ParentComponent);
		Entity Children[MAX_CHILD_COMPONENTS];
		uint32 ChildCount = 0;
	};
}