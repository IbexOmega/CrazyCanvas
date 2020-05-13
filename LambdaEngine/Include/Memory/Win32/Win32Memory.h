#pragma once

#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Memory/API/Memory.h"

namespace LambdaEngine
{
	class Win32Memory : public Memory
	{
	public:
		DECL_STATIC_CLASS(Win32Memory);

		static void*	VirtualAlloc(uint64 sizeInBytes);
		static bool		VirtualProtect(void* pMemory);
		static bool		VirtualFree(void* pMemory);
	};

	typedef Win32Memory PlatformMemory;
}

#endif