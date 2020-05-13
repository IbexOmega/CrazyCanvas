#pragma once
#include "LambdaEngine.h"

#define DBG_NEW new

namespace LambdaEngine
{
	enum FMemoryDebugFlags : uint32
	{
		MEMORY_DEBUG_FLAGS_NONE				= 0,
		MEMORY_DEBUG_FLAGS_OVERFLOW_PROTECT	= FLAG(1),
		MEMORY_DEBUG_FLAGS_LEAK_CHECK		= FLAG(2),
	};

	class LAMBDA_API MemoryAllocator
	{
	public:
		DECL_STATIC_CLASS(MemoryAllocator);
		
		static void* Malloc(uint64 sizeInBytes);
		static void* Malloc(uint64 sizeInBytes, uint64 alignment);
		
		static void Free(void* pPtr);
		
		static void SetDebugFlags(uint32 debugFlags);

	private:
		static uint32 s_DebugFlags;
	};
}