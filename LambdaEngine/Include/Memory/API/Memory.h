#pragma once
#include "LambdaEngine.h"

#ifdef LAMBDA_VISUAL_STUDIO
	#pragma warning(push)
	#pragma warning(disable : 4100) // Disable unreferenced variable warning
#endif

namespace LambdaEngine
{
	class Memory
	{
	public:
		DECL_STATIC_CLASS(Memory);
		
		static void*	VirtualAlloc(uint64 sizeInBytes)						{ return nullptr; }
		static bool		VirtualProtect(void* pMemory, uint64 sizeInBytes)		{ return false; }
		static bool		VirtualFree(void* pMemory)								{ return false; }

		static uint64 GetPageSize()					{ return 0; }
		static uint64 GetAllocationGranularity()	{ return 0; }
	};
}


#ifdef LAMBDA_VISUAL_STUDIO
	#pragma warning(pop)
#endif