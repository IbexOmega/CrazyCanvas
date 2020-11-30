#pragma once

#include "ECS/Component.h"
#include "ECS/Entity.h"

namespace LambdaEngine
{
	class Texture;
	class Buffer;
	class TextureView;

	struct MeshPaintComponent
	{
		DECL_COMPONENT(MeshPaintComponent);

	};

	namespace MeshPaint
	{
		MeshPaintComponent CreateComponent(Entity entity);
	}
}