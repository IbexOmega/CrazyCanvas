#pragma once
#include "Defines.h"

LAMBDA_API void HandleAssert(const char* pFile, int line);
LAMBDA_API void HandleAssertWithMessage(const char* pFile, int line, const char* pMessageFormat, ...);

/*
* DEBUGBREAK stops the execution of the application
*/
#ifdef LAMBDA_VISUAL_STUDIO
	#define DEBUGBREAK(...) __debugbreak()
#else
	#include <stdlib.h>
	#define DEBUGBREAK(...) abort()
#endif

#ifndef LAMBDA_ENABLE_ASSERTS
	#ifdef LAMBDA_DEBUG
		#define LAMBDA_ENABLE_ASSERTS 1
	#else
		#define LAMBDA_ENABLE_ASSERTS 0
	#endif
#endif

#ifdef LAMBDA_VISUAL_STUDIO
	#pragma warning(error : 4002)
#endif

#if LAMBDA_ENABLE_ASSERTS
	#define ASSERT(condition) \
		if (!(condition)) \
		{ \
			HandleAssert(__FILE__, __LINE__); \
			DEBUGBREAK(); \
		}

	#define ASSERT_MSG(condition, ...) \
		if (!(condition)) \
		{ \
			HandleAssertWithMessage(__FILE__, __LINE__, __VA_ARGS__); \
			DEBUGBREAK(); \
		}
#else
	#define ASSERT(condition)
	#define ASSERT_MSG(condition, ...)
#endif

#define VALIDATE(condition)             ASSERT(condition)
#define VALIDATE_MSG(condition, ...)    ASSERT_MSG(condition, __VA_ARGS__)
