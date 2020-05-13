#include "Memory/API/MemoryAllocator.h"
#include "Memory/API/PlatformMemory.h"

#include <stdlib.h>

namespace LambdaEngine
{
	uint32 MemoryAllocator::s_DebugFlags = 0;

	void* MemoryAllocator::Malloc(uint64 sizeInBytes)
	{
		if (s_DebugFlags & MEMORY_DEBUG_FLAGS_OVERFLOW_PROTECT)
		{
			uint64 pageSize		= PlatformMemory::GetPageSize();
			uint64 numPages		= (sizeInBytes / pageSize) + 2;
			uint64 totalSize	= numPages * pageSize;
			uint64 padding		= totalSize - pageSize - sizeInBytes;
			
			// Allocate one page for allocation and one protection page
			byte* pMemory = reinterpret_cast<byte*>(PlatformMemory::VirtualAlloc(totalSize));
			
			uint64	usableSize		= totalSize - pageSize;
			byte*	pProtectionPage	= pMemory + (usableSize);
			bool result = PlatformMemory::VirtualProtect(pProtectionPage, pageSize);
			if (!result)
			{
				return nullptr;
			}
			else
			{
				return reinterpret_cast<void*>(pMemory + padding);
			}
		}
		else
		{
			return malloc(sizeInBytes);
		}
	}

	void* MemoryAllocator::Malloc(uint64 sizeInBytes, uint64 alignment)
	{
		return _aligned_malloc(sizeInBytes, alignment);
	}

	void MemoryAllocator::Free(void* pPtr)
	{
		if (s_DebugFlags & MEMORY_DEBUG_FLAGS_OVERFLOW_PROTECT)
		{
			PlatformMemory::VirtualFree(pPtr);
		}
		else
		{
			free(pPtr);
		}
	}
	
	void MemoryAllocator::SetDebugFlags(uint32 debugFlags)
	{
#ifdef LAMBDA_PLATFORM_WINDOWS
		if (debugFlags & MEMORY_DEBUG_FLAGS_LEAK_CHECK)
		{
			_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
		}
#endif

		s_DebugFlags = debugFlags;
	}
}
