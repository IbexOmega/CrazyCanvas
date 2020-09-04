#include "Memory/API/Malloc.h"
#include "Memory/API/PlatformMemory.h"

#include "Application/API/PlatformMisc.h"

#include "Math/Math.h"

#include <stdlib.h>

#ifdef LAMBDA_VISUAL_STUDIO
	#include <crtdbg.h>
#endif

#ifdef LAMBDA_PRODUCTION
	#define DISABLE_MEM_DEBUG
#endif

#define DISABLE_MEM_DEBUG
#ifdef DISABLE_MEM_DEBUG
	#define MEM_DEBUG_ENABLED 0
#else
	#define MEM_DEBUG_ENABLED 1
#endif

#ifdef LAMBDA_PLATFORM_WINDOWS
	#define aligned_malloc(sizeInBytes, alignment) 				malloc(sizeInBytes); (void)alignment

	#define debug_malloc(sizeInBytes, pFileName, lineNumber)	_malloc_dbg(sizeInBytes, _NORMAL_BLOCK, pFileName, lineNumber); (void)pFileName; (void)lineNumber
#elif defined(LAMBDA_PLATFORM_MACOS)
	#define aligned_malloc(sizeInBytes, alignment) 				aligned_alloc(sizeInBytes, alignment)

	#define debug_malloc(sizeInBytes, pFileName, lineNumber)	 malloc(sizeInBytes); (void)pFileName; (void)lineNumber
#endif

#define ALLOCATION_HEADER_SIZE __STDCPP_DEFAULT_NEW_ALIGNMENT__

/*
* Custom memory handler
*/
namespace LambdaEngine
{
	uint16 Malloc::s_DebugFlags = 0;

	void* Malloc::Allocate(uint64 sizeInBytes)
	{
#if MEM_DEBUG_ENABLED
		return Allocate(sizeInBytes, __STDCPP_DEFAULT_NEW_ALIGNMENT__);
#else
		return malloc(sizeInBytes);
#endif
	}

	void* Malloc::Allocate(uint64 sizeInBytes, uint64 alignment)
	{
#if MEM_DEBUG_ENABLED
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

		const int64 padding 			= reinterpret_cast<byte*>(pAlignedAddress) - pMemory;
#if 0
		const uint64 address			= reinterpret_cast<uint64>(pMemory);
		const uint64 alignedAddress 	= reinterpret_cast<uint64>(pAlignedAddress);
		const uint64 addressEnd			= address + alignedSize;
		const uint64 alignedAddressEnd 	= alignedAddress + sizeInBytes;
		
		VALIDATE(alignedAddressEnd <= addressEnd);
	
		PlatformMisc::OutputDebugString("Allocating memory: size=%llu, sizeWithHeader=%llu, alignedSize=%llu, padding=%u", sizeInBytes, sizeWithHeader, alignedSize, padding);
		PlatformMisc::OutputDebugString("Adress=%llu, AdressEnd=%llu, AlignedAdress=%llu, AlignedAdressEnd=%llu,", address, addressEnd, alignedAddress, alignedAddressEnd);
#endif
		
		SetAllocationFlags(pAlignedAddress, uint16(padding));
		return pAlignedAddress;
#else
		return aligned_malloc(sizeInBytes, alignment);
#endif
	}

	void* Malloc::AllocateDbg(uint64 sizeInBytes, const char* pFileName, int32 lineNumber)
	{
#if MEM_DEBUG_ENABLED
		return AllocateDbg(sizeInBytes, __STDCPP_DEFAULT_NEW_ALIGNMENT__, pFileName, lineNumber);
#else
		return debug_malloc(sizeInBytes, pFileName, lineNumber);
#endif
	}

	void* Malloc::AllocateDbg(uint64 sizeInBytes, uint64 alignment, const char* pFileName, int32 lineNumber)
	{
#if MEM_DEBUG_ENABLED
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

		const int64 padding = reinterpret_cast<byte*>(pAlignedAddress) - pMemory;
		SetAllocationFlags(pAlignedAddress, uint16(padding));

		return pAlignedAddress;
#else
		UNREFERENCED_VARIABLE(pFileName);
		UNREFERENCED_VARIABLE(lineNumber);

		return aligned_malloc(sizeInBytes, alignment);
#endif
	}

	void Malloc::Free(void* pPtr)
	{
#if MEM_DEBUG_ENABLED
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
#else
		free(pPtr);
#endif
	}
	
	void Malloc::SetDebugFlags(uint16 debugFlags)
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
		// Get address to flags
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
