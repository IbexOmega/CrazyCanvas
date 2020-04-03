#pragma once

#include "LambdaEngine.h"

namespace LambdaEngine
{
	class SoundEffect3D;

	enum ESoundFlags : uint32
	{
		NONE	= BIT(0),
		LOOPING = BIT(1),
	};
}