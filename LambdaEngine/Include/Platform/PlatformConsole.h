#pragma once

#ifdef LAMBDA_PLATFORM_WINDOWS
	#include "Win32/Win32Console.h"
#elif defined(LAMBDA_PLATFORM_MACOS)
    #include "macOS/MacConsole.h"
#else
    #error No platform defined
#endif
