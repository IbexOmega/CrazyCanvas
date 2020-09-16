#pragma once

#include "ECS/Component.h"

#include <typeindex>

namespace LambdaEngine
{
	struct MeshComponent
	{
		GUID_Lambda Mesh;
		GUID_Lambda Material;
	};

	struct StaticMeshComponent : MeshComponent {};
	struct DynamicMeshComponent : MeshComponent {};

	const std::type_index g_TIDStaticMeshComponent = TID(StaticMeshComponent);
	const std::type_index g_TIDDynamicMeshComponent = TID(DynamicMeshComponent);
}

