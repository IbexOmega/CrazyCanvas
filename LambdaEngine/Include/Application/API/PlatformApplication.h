#pragma once

#ifdef LAMBDA_PLATFORM_WINDOWS
	#include "Application/Win32/Win32Application.h"
#elif defined(LAMBDA_PLATFORM_MACOS)
	#include "Application/Mac/MacApplication.h"
#else
	#error No platform defined
#endif
