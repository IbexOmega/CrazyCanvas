#pragma once
#include "Types.h"
#include "Defines.h"

#include <new>

#ifdef LAMBDA_DEBUG
	#define DBG_NEW	new(__FILE__, __LINE__)
#else
	#define DBG_NEW	new
#endif

/*
* Delete and Release
*/
#define DELETE_OBJECT(object)	delete(object); (object) = nullptr
#define SAFEDELETE(object)		if ((object))	{ DELETE_OBJECT(object); }

#define DELETE_ARRAY(array)		delete[](array); (array) = nullptr
#define SAFEDELETE_ARRAY(array)	if ((array))	{ DELETE_ARRAY(array); }

#define RELEASE(object)			(object)->Release(); (object) = nullptr
#define SAFERELEASE(object)		if ((object))	{ RELEASE(object); }

/*
* Custom memory handler
*/
namespace LambdaEngine
{
	enum FMemoryDebugFlags : uint16
	{
		MEMORY_DEBUG_FLAGS_NONE				= 0,
		MEMORY_DEBUG_FLAGS_OVERFLOW_PROTECT	= FLAG(1),
		MEMORY_DEBUG_FLAGS_LEAK_CHECK		= FLAG(2),
	};

	class LAMBDA_API Malloc
	{
	public:
		DECL_STATIC_CLASS(Malloc);
		
		static void* Allocate(uint64 sizeInBytes);
		static void* Allocate(uint64 sizeInBytes, uint64 alignment);

		static void* AllocateDbg(uint64 sizeInBytes, const char* pFileName, int32 lineNumber);
		static void* AllocateDbg(uint64 sizeInBytes, uint64 alignment, const char* pFileName, int32 lineNumber);
		
		static void Free(void* pPtr);
		
		static void SetDebugFlags(uint16 debugFlags);

	private:
		static void* AllocateProtected(uint64 sizeInBytes);
		static void* AlignAddress(void* pAddress, uint64 alignment);

		static void SetAllocationFlags(void* pAllocation, uint16 padding);
		static uint16 GetAllocationFlags(void* pAllocation);
		static uint16 GetAllocationPadding(void* pAllocation);

	private:
		static uint16 s_DebugFlags;
	};
}

#ifdef LAMBDA_VISUAL_STUDIO
	#pragma warning(push)
	#pragma warning(disable : 4595) // Disable "non-member operator new or delete functions may not be declared inline" warning
#endif

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