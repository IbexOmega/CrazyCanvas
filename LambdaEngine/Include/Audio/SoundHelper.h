#pragma once

#include "LambdaEngine.h"

namespace LambdaEngine
{
	enum ESoundFlags : uint32
	{
		NONE = BIT(0),
	};

	struct SoundDesc
	{
		const char* pName = "";
		const void* pData = nullptr;
		uint32 DataSize = 0;

		ESoundFlags Flags = ESoundFlags::NONE;
	};
}