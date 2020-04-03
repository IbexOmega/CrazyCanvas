#pragma once

#include "LambdaEngine.h"

namespace LambdaEngine
{
	class SoundEffect3D;

	enum ESoundModeFlags : uint32
	{
		SOUND_MODE_NONE		= BIT(0),
		SOUND_MODE_LOOPING	= BIT(1),
	};
}