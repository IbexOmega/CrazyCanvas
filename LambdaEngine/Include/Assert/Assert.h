#pragma once

#ifdef LAMBDA_DEBUG
    #ifdef LAMBDA_PLATFORM_WINDOWS
        #define ASSERT(condition) if (!condition) { _debugbreak(); }
    #elif defined(LAMBDA_PLATFORM_MACOS)
        #define ASSERT(condition) if (!condition) { abort(); }
    #endif
#else
    #define ASSERT(condition)
#endif
