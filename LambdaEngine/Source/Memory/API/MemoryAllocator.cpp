#include "Memory/API/MemoryAllocator.h"
#include "Memory/API/PlatformMemory.h"

namespace LambdaEngine
{
	uint32 MemoryAllocator::s_DebugFlags = 0;

	void* MemoryAllocator::Malloc(uint64 sizeInBytes)
	{
		if (s_DebugFlags & MEMORY_DEBUG_FLAGS_BUFFER_OVERFLOW_PROTECT)
		{
			void* pMemory = PlatformMemory::VirtualAlloc(sizeInBytes);
		}
		else
		{
			return malloc(sizeInBytes);
		}
	}

	void* MemoryAllocator::Malloc(uint64 sizeInBytes, uint64 alignment)
	{
		return nullptr;
	}

	void MemoryAllocator::Free(void* pPtr)
	{
	}
	
	void MemoryAllocator::SetDebugFlags(uint32 debugFlags)
	{
		s_DebugFlags = debugFlags;
	}
}
