#pragma once

#include "Rendering/Core/API/GraphicsTypes.h"
#include "NsRender/RenderDevice.h"

namespace LambdaEngine
{
	EFormat NoesisFormatToLamdaFormat(Noesis::TextureFormat::Enum format)
	{
		switch (format)
		{
		case Noesis::TextureFormat::Enum::R8:		return EFormat::FORMAT_R8_UNORM;
		case Noesis::TextureFormat::Enum::RGBA8:	return EFormat::FORMAT_R8G8B8A8_UNORM;
		}

		return EFormat::FORMAT_R8G8B8A8_UNORM;
	}
}