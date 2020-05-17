#include "Memory/API/Malloc.h"
#include "Memory/API/PlatformMemory.h"

#include "Math/Math.h"

#include <stdlib.h>

#ifdef LAMBDA_VISUAL_STUDIO
	#include <crtdbg.h>
#endif

#ifdef LAMBDA_PLATFORM_WINDOWS
	#define debug_malloc(sizeInBytes, pFileName, lineNumber)	_malloc_dbg(sizeInBytes, _NORMAL_BLOCK, pFileName, lineNumber)
#elif defined(LAMBDA_PLATFORM_MACOS)
	#define debug_malloc(sizeInBytes, pFileName, lineNumber)	 malloc(sizeInBytes); (void)pFileName; (void)lineNumber
#endif

#define ALLOCATION_HEADER_SIZE __STDCPP_DEFAULT_NEW_ALIGNMENT__

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

/*
* Custom memory handler
*/
namespace LambdaEngine
{
	uint16 Malloc::s_DebugFlags = 0;

	void* Malloc::Allocate(uint64 sizeInBytes)
	{
		return Allocate(sizeInBytes, __STDCPP_DEFAULT_NEW_ALIGNMENT__);
	}

	void* Malloc::Allocate(uint64 sizeInBytes, uint64 alignment)
	{
		if (sizeInBytes == 0)
		{
			return nullptr;
		}

		if (alignment < __STDCPP_DEFAULT_NEW_ALIGNMENT__)
		{
			alignment = __STDCPP_DEFAULT_NEW_ALIGNMENT__;
		}

		const uint64 sizeWithHeader = sizeInBytes + ALLOCATION_HEADER_SIZE;
		const uint64 alignedSize	= AlignUp(sizeWithHeader, alignment);

		void* pResult = nullptr;
		if (s_DebugFlags & MEMORY_DEBUG_FLAGS_OVERFLOW_PROTECT)
		{
			pResult = AllocateProtected(alignedSize);
		}
		else
		{
			pResult = malloc(alignedSize);
		}

		byte* const pMemory			= reinterpret_cast<byte*>(pResult);
		byte* const pAllocation		= pMemory + ALLOCATION_HEADER_SIZE;
		void* const pAlignedAddress = AlignAddress(pAllocation, __STDCPP_DEFAULT_NEW_ALIGNMENT__);

		const uint16 padding = reinterpret_cast<byte*>(pAlignedAddress) - pMemory;
		SetAllocationFlags(pAlignedAddress, padding);

		return pAlignedAddress;
	}

	void* Malloc::AllocateDbg(uint64 sizeInBytes, const char* pFileName, int32 lineNumber)
	{
		return AllocateDbg(sizeInBytes, __STDCPP_DEFAULT_NEW_ALIGNMENT__, pFileName, lineNumber);
	}

	void* Malloc::AllocateDbg(uint64 sizeInBytes, uint64 alignment, const char* pFileName, int32 lineNumber)
	{
		if (sizeInBytes == 0)
		{
			return nullptr;
		}

		if (alignment < __STDCPP_DEFAULT_NEW_ALIGNMENT__)
		{
			alignment = __STDCPP_DEFAULT_NEW_ALIGNMENT__;
		}

		const uint64 sizeWithHeader = sizeInBytes + ALLOCATION_HEADER_SIZE;
		const uint64 alignedSize	= AlignUp(sizeWithHeader, alignment);

		void* pResult = nullptr;
		if (s_DebugFlags & MEMORY_DEBUG_FLAGS_OVERFLOW_PROTECT)
		{
			pResult = AllocateProtected(alignedSize);
		}
		else
		{
			pResult = debug_malloc(alignedSize, pFileName, lineNumber);
		}

		byte* const pMemory			= reinterpret_cast<byte*>(pResult);
		byte* const pAllocation		= pMemory + ALLOCATION_HEADER_SIZE;
		void* const pAlignedAddress = AlignAddress(pAllocation, __STDCPP_DEFAULT_NEW_ALIGNMENT__);

		const uint16 padding = reinterpret_cast<byte*>(pAlignedAddress) - pMemory;
		SetAllocationFlags(pAlignedAddress, padding);

		return pAlignedAddress;
	}

	void Malloc::Free(void* pPtr)
	{
		// It appears that it is legal that free recives a nullptr so we support is aswell
		if (pPtr == nullptr)
		{
			return;
		}

		const uint16 flags 		= GetAllocationFlags(pPtr);
		const uint16 padding 	= GetAllocationPadding(pPtr);
		
		byte* const pBytes		= reinterpret_cast<byte*>(pPtr);
		byte* const pAllocation = pBytes - padding;
		if (flags & MEMORY_DEBUG_FLAGS_OVERFLOW_PROTECT)
		{
			PlatformMemory::VirtualFree(pAllocation);
		}
		else
		{
			free(pAllocation);
		}
	}
	
	void Malloc::SetDebugFlags(uint32 debugFlags)
	{
#ifdef LAMBDA_PLATFORM_WINDOWS
		if (debugFlags & MEMORY_DEBUG_FLAGS_LEAK_CHECK)
		{
			_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
		}
#endif

		s_DebugFlags = debugFlags;
	}
	
	void* Malloc::AllocateProtected(uint64 sizeInBytes)
	{
		const uint64 pageSize		= PlatformMemory::GetPageSize();
		const uint64 numPages		= (sizeInBytes / pageSize) + 2;
		const uint64 totalSize		= numPages * pageSize;
		const uint64 padding		= totalSize - pageSize - sizeInBytes;

		// Allocate one page for allocation and one protection page
		byte* const pMemory = reinterpret_cast<byte*>(PlatformMemory::VirtualAlloc(totalSize));

		uint64		usableSize		= totalSize - pageSize;
		byte* const pProtectionPage = pMemory + (usableSize);
		if (PlatformMemory::VirtualProtect(pProtectionPage, pageSize))
		{
			return reinterpret_cast<void*>(pMemory + padding);
		}
		else
		{
			return nullptr;
		}
	}

	void* Malloc::AlignAddress(void* pAddress, uint64 alignment)
	{
		const uint64 address		= reinterpret_cast<uint64>(pAddress);
		const uint64 alignedAddress = AlignUp(address, alignment);
		return reinterpret_cast<void*>(alignedAddress);
	}
	
	void Malloc::SetAllocationFlags(void* pAllocation, uint16 padding)
	{
		VALIDATE(padding >= ALLOCATION_HEADER_SIZE);

		// Get address to flags
		byte* pMemory = reinterpret_cast<byte*>(pAllocation) - sizeof(uint16);
		
		uint16* const pFlagsPtr = reinterpret_cast<uint16*>(pMemory);
		(*pFlagsPtr) = uint16(s_DebugFlags);
		
		// Get address to padding
		pMemory = reinterpret_cast<byte*>(pMemory) - sizeof(uint16);
		
		uint16* const pSizePtr = reinterpret_cast<uint16*>(pMemory);
		(*pSizePtr) = padding;
	}
	
	uint16 Malloc::GetAllocationFlags(void* pAllocation)
	{
		// Get address to padding
		byte* pMemory = reinterpret_cast<byte*>(pAllocation) - sizeof(uint16);
		
		uint16* const pFlagsPtr = reinterpret_cast<uint16*>(pMemory);
		const uint16 flags = uint16(*pFlagsPtr);
		
		return flags;
	}

	uint16 Malloc::GetAllocationPadding(void* pAllocation)
	{
		// Get address to padding
		constexpr uint16 offset = sizeof(uint16) * 2;
		byte* pMemory = reinterpret_cast<byte*>(pAllocation) - offset;
		
		uint16* const pSizePtr = reinterpret_cast<uint16*>(pMemory);
		const uint16 padding = uint16(*pSizePtr);
		
		return padding;
	}
}
