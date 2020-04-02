#pragma once

#ifdef LAMBDA_PLATFORM_WINDOWS
	#include "Application/Win32/Win32Misc.h"
#elif defined(LAMBDA_PLATFORM_MACOS)
	#include "Application/Mac/MacMisc.h"
#else
	#error No platform defined
#endif
