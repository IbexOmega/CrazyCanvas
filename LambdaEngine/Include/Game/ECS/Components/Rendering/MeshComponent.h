#pragma once

#include "ECS/Component.h"

#include <typeindex>

namespace LambdaEngine
{
	struct MeshComponent
	{
		DECL_COMPONENT(MeshComponent);
		GUID_Lambda MeshGUID;
		GUID_Lambda MaterialGUID;
	};

	struct StaticComponent
	{
		DECL_COMPONENT(StaticComponent);
	};

	struct DynamicComponent
	{
		DECL_COMPONENT(DynamicComponent);
	};
}

