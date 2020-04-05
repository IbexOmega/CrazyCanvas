#pragma once
#include "Defines.h"

LAMBDA_API void HandleAssert(const char* pLine, int line);

/*
* DEBUGBREAK stops the execution of the application
*/
#ifdef LAMBDA_VISUAL_STUDIO
    #define DEBUGBREAK(...) __debugbreak()
#else
    #include <stdlib.h>
    #define DEBUGBREAK(...) abort()
#endif

#ifdef LAMBDA_DEBUG
    #define ASSERT(condition) \
        if (!(condition)) \
        { \
            HandleAssert(__FILE__, __LINE__); \
            DEBUGBREAK(); \
        }
#else
    #define ASSERT(condition)
#endif
