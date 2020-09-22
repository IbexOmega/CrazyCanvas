#pragma once

#include "ECS/Component.h"

namespace LambdaEngine
{
	struct MeshPaintComponent
	{
		DECL_DRAW_ARG_COMPONENT(MeshPaintComponent);
		GUID_Lambda UnwrappedTexture; // Holds the unwrapped texture of the mesh.
	};
}