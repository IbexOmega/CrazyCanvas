#pragma once
#ifdef LAMBDA_PLATFORM_WINDOWS
	#include "Win32/Win32Application.h"
#else
	#error No platform defined
#endif