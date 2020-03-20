#pragma once
#ifdef LAMBDA_PLATFORM_WINDOWS
	#include "Win32/Win32Application.h"
#elif defined(LAMBDA_PLATFORM_MACOS)
	#include "macOS/MacApplication.h"
#else
	#error No platform defined
#endif
