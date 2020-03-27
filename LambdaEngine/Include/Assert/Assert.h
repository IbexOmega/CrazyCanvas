#pragma once
#include "Defines.h"

LAMBDA_API void Assert(const char* pLine, int line);

#ifdef LAMBDA_VISUAL_STUDIO
    #define DEBUGBREAK(...) __debugbreak()
#else
    #define DEBUGBREAK(...) abort()
#endif

#ifdef LAMBDA_DEBUG
    #define ASSERT(condition) \
        if (!(condition)) \
        { \
            Assert(__FILE__, __LINE__); \
            DEBUGBREAK(); \
        }
#else
    #define ASSERT(condition)
#endif
