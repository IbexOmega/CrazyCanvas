#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#include "Memory/API/Memory.h"

namespace LambdaEngine
{
	class MacMemory : public Memory
	{
	public:
		DECL_STATIC_CLASS(MacMemory);
		
		static void* 	VirtualAlloc(uint64 sizeInBytes);
		static bool		VirtualProtect(void* pMemory, uint64 sizeInBytes);
		static bool		VirtualFree(void* pMemory);

		static uint64 GetPageSize();
		static uint64 GetAllocationGranularity();
	};

	typedef MacMemory PlatformMemory;
}

#endif
