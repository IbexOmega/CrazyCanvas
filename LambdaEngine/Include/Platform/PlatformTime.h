#pragma once

#ifdef LAMBDA_PLATFORM_WINDOWS
	#include "Win32/Win32Time.h"
#elif defined(LAMBDA_PLATFORM_MACOS)
	#include "macOS/MacTime.h"
#else
	#error No platform defined
#endif