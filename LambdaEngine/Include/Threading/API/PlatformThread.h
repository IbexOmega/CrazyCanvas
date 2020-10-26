#pragma once
#ifdef LAMBDA_PLATFORM_WINDOWS
	#include "Threading/Win32/Win32Thread.h"
#else
	#error "Not defined for platform"
#endif