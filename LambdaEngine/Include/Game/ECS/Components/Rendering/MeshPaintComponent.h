#pragma once

#include "ECS/Component.h"
#include "ECS/Entity.h"

namespace LambdaEngine
{
	struct MeshPaintComponent
	{
		DECL_COMPONENT(MeshPaintComponent);
		GUID_Lambda UnwrappedTexture; // Holds the unwrapped mask texture of the mesh.
	};

	namespace MeshPaint
	{
		MeshPaintComponent CreateComponent(Entity entity, const std::string& textureName, uint32 width, uint32 height);
	}
}