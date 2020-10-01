#pragma once
#include <new>

#ifdef LAMBDA_DEBUG
	#define DBG_NEW	new(__FILE__, __LINE__)
#else
	#define DBG_NEW	new
#endif

/*
* Custom Debug new and delete
*/

void* operator new  (size_t sizeInBytes, const char* pFileName, int32 lineNumber);
void* operator new[](size_t sizeInBytes, const char* pFileName, int32 lineNumber);

void operator delete  (void* pPtr, const char*, int32) noexcept;
void operator delete[](void* pPtr, const char*, int32) noexcept;

/*
* Custom new and delete
*/

void* operator new  (size_t sizeInBytes);
void* operator new[](size_t sizeInBytes);
void* operator new  (size_t sizeInBytes, const std::nothrow_t&)	noexcept;
void* operator new[](size_t sizeInBytes, const std::nothrow_t&)	noexcept;

void operator delete  (void* pPtr)			noexcept;
void operator delete[](void* pPtr)			noexcept;
void operator delete  (void* pPtr, size_t)	noexcept;
void operator delete[](void* pPtr, size_t)	noexcept;

#ifdef LAMBDA_SHARED_LIB
	#ifdef LAMBDA_VISUAL_STUDIO
		#pragma warning(push)
		#pragma warning(disable : 4595) // Disable "non-member operator new or delete functions may not be declared inline" warning
	#endif

#include "Malloc.h"

/*
* Custom Debug new and delete
*/

inline void* operator new(size_t sizeInBytes, const char* pFileName, int32 lineNumber)
{
	return LambdaEngine::Malloc::AllocateDbg(sizeInBytes, pFileName, lineNumber);
}

inline void* operator new[](size_t sizeInBytes, const char* pFileName, int32 lineNumber)
{
	return LambdaEngine::Malloc::AllocateDbg(sizeInBytes, pFileName, lineNumber);
}

inline void operator delete(void* pPtr, const char*, int32) noexcept
{
	LambdaEngine::Malloc::Free(pPtr);
}

inline void operator delete[](void* pPtr, const char*, int32) noexcept
{
	LambdaEngine::Malloc::Free(pPtr);
}

/*
* Custom new and delete
*/

inline void* operator new(size_t sizeInBytes)
{
	return LambdaEngine::Malloc::Allocate(sizeInBytes);
}

inline void* operator new[](size_t sizeInBytes)
{
	return LambdaEngine::Malloc::Allocate(sizeInBytes);
}

inline void* operator new(size_t sizeInBytes, const std::nothrow_t&) noexcept
{
	return LambdaEngine::Malloc::Allocate(sizeInBytes);
}

inline void* operator new[](size_t sizeInBytes, const std::nothrow_t&) noexcept
{
	return LambdaEngine::Malloc::Allocate(sizeInBytes);
}

inline void operator delete(void* pPtr) noexcept
{
	LambdaEngine::Malloc::Free(pPtr);
}

inline void operator delete[](void* pPtr) noexcept
{
	LambdaEngine::Malloc::Free(pPtr);
}

inline void operator delete(void* pPtr, size_t) noexcept
{
	LambdaEngine::Malloc::Free(pPtr);
}

inline void operator delete[](void* pPtr, size_t) noexcept
{
	LambdaEngine::Malloc::Free(pPtr);
}

	#ifdef LAMBDA_VISUAL_STUDIO
		#pragma warning(pop)
	#endif
#endif