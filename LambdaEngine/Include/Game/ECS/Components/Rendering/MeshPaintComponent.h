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
		Buffer*			pReadBackBuffer		= nullptr;
		Texture*		pTexture			= nullptr;
		TextureView*	pTextureView		= nullptr;
	};

	namespace MeshPaint
	{
		MeshPaintComponent CreateComponent(
			Entity entity, 
			const std::string& textureName,
			uint32 width, 
			uint32 height,
			bool generateMips,
			bool createMipReadBack = false);
	}
}