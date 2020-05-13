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
		static bool		VirtualProtect(void* pMemory);
	};
}

#endif
