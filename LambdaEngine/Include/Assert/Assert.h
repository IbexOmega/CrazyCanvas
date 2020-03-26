#pragma once
#include "Defines.h"

LAMBDA_API void Assert();

#ifdef LAMBDA_VISUAL_STUDIO
    #define DEBUGBREAK(...) __debugbreak()
#else
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
