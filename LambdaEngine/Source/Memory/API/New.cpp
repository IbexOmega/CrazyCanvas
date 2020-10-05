#ifndef LAMBDA_SHARED_LIB
#include "Memory/API/New.h"

/*
* Custom Debug new and delete
*/

void* operator new(size_t sizeInBytes, const char* pFileName, int32 lineNumber)
{
	return LambdaEngine::Malloc::AllocateDbg(sizeInBytes, pFileName, lineNumber);
}

void* operator new[](size_t sizeInBytes, const char* pFileName, int32 lineNumber)
{
	return LambdaEngine::Malloc::AllocateDbg(sizeInBytes, pFileName, lineNumber);
}

void operator delete(void* pPtr, const char*, int32) noexcept
{
	LambdaEngine::Malloc::Free(pPtr);
}

void operator delete[](void* pPtr, const char*, int32) noexcept
{
	LambdaEngine::Malloc::Free(pPtr);
}

/*
* Custom new and delete
*/

void* operator new(size_t sizeInBytes)
{
	return LambdaEngine::Malloc::Allocate(sizeInBytes);
}

void* operator new[](size_t sizeInBytes)
{
	return LambdaEngine::Malloc::Allocate(sizeInBytes);
}

void* operator new(size_t sizeInBytes, const std::nothrow_t&) noexcept
{
	return LambdaEngine::Malloc::Allocate(sizeInBytes);
}

void* operator new[](size_t sizeInBytes, const std::nothrow_t&) noexcept
{
	return LambdaEngine::Malloc::Allocate(sizeInBytes);
}

void operator delete(void* pPtr) noexcept
{
	LambdaEngine::Malloc::Free(pPtr);
}

void operator delete[](void* pPtr) noexcept
{
	LambdaEngine::Malloc::Free(pPtr);
}

void operator delete(void* pPtr, size_t) noexcept
{
	LambdaEngine::Malloc::Free(pPtr);
}

void operator delete[](void* pPtr, size_t) noexcept
{
	LambdaEngine::Malloc::Free(pPtr);
}

#endif