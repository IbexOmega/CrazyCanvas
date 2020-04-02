#pragma once

#ifdef LAMBDA_PLATFORM_WINDOWS
	#include "Time/Win32/Win32Time.h"
#elif defined(LAMBDA_PLATFORM_MACOS)
	#include "Time/Mac/MacTime.h"
#else
	#error No platform defined
#endif
