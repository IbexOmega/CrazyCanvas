#pragma once

#include "ECS/Component.h"

#include <typeindex>

namespace LambdaEngine
{
	struct MeshComponent
	{
		GUID_Lambda MeshGUID;
		GUID_Lambda MaterialGUID;
	};

	struct StaticMeshComponent : MeshComponent { DECL_COMPONENT(StaticMeshComponent); };
	struct DynamicMeshComponent : MeshComponent { DECL_COMPONENT(DynamicMeshComponent); };
}

