#pragma once

#include "ECS/Component.h"

namespace LambdaEngine
{
	struct MeshPaintComponent
	{
		DECL_COMPONENT(MeshPaintComponent);
		GUID_Lambda UnwrappedTexture; // Holds the unwrapped mask texture of the mesh.
	};
}