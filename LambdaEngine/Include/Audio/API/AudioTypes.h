#pragma once

#include "LambdaEngine.h"

namespace LambdaEngine
{
	class SoundEffect3DFMOD;

	enum FSoundModeFlags : uint32
	{
		SOUND_MODE_NONE		= FLAG(0),
		SOUND_MODE_LOOPING	= FLAG(1),
	};
}