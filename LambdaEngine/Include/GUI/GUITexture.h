#pragma once

#include "NsRender/Texture.h"

namespace LambdaEngine
{
	class GUITexture : public Noesis::Texture
	{
		GUITexture();
		~GUITexture();

		/// Returns the width of the texture
		virtual uint32_t GetWidth() const override final;

		/// Returns the height of the texture
		virtual uint32_t GetHeight() const override final;

		/// True if the texture has mipmaps
		virtual bool HasMipMaps() const override final;

		/// True is the texture must be vertically inverted when mapped. This is true for render targets
		/// on platforms (OpenGL) where texture V coordinate is zero at the "bottom of the texture"
		virtual bool IsInverted() const override final;
	};
}
