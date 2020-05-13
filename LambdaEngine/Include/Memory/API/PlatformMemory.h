#pragma once

#ifdef LAMBDA_PLATFORM_WINDOWS
	#include "Memory/Win32/Win32Memory.h"
#elif defined(LAMBDA_PLATFORM_MACOS)
	#include "Memory/Mac/MacMemory.h"
#else
	#error No platform defined
#endif
