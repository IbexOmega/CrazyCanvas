#pragma once

#include "ECS/Component.h"

namespace LambdaEngine
{
	struct MeshComponent
	{
		DECL_COMPONENT(MeshComponent);
		GUID_Lambda MeshGUID;
		GUID_Lambda MaterialGUID;
	};
}

