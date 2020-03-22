#pragma once

#ifdef LAMBDA_PLATFORM_WINDOWS
    //TODO: Implement on windows
#elif defined(LAMBDA_PLATFORM_MACOS)
    #include "macOS/MacConsole.h"
#else
    #error No platform defined
#endif
