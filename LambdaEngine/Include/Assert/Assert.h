#pragma once
#include "Defines.h"

LAMBDA_API void Assert();

#ifdef LAMBDA_PLATFORM_WINDOWS
    #define DEBUGBREAK(...) __debugbreak()
#elif defined(LAMBDA_PLATFORM_MACOS)
    #define DEBUGBREAK(...) abort()
#endif

#ifdef LAMBDA_DEBUG
    #define ASSERT(condition) \
        if (!(condition)) \
        { \
            Assert(); \
            DEBUGBREAK(); \
        }
#else
    #define ASSERT(condition)
#endif
