#include "GUI/GUITexture.h"

namespace LambdaEngine
{
	GUITexture::GUITexture()
	{
	}

	GUITexture::~GUITexture()
	{
	}

	uint32_t GUITexture::GetWidth() const
	{
		return uint32_t();
	}

	uint32_t GUITexture::GetHeight() const
	{
		return uint32_t();
	}

	bool GUITexture::HasMipMaps() const
	{
		return false;
	}

	bool GUITexture::IsInverted() const
	{
		return false;
	}
}