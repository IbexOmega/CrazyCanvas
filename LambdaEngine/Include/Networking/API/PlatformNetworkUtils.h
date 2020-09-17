#pragma once

#ifdef LAMBDA_PLATFORM_WINDOWS
	#include "Networking/Win32/Win32NetworkUtils.h"
#elif defined(LAMBDA_PLATFORM_MACOS)
    #include "Networking/Mac/MacNetworkUtils.h"
#else
	#error No platform defined
#endif
