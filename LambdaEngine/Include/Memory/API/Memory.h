#include "LambdaEngine.h"

namespace LambdaEngine
{
	class Memory
	{
	public:
		DECL_STATIC_CLASS(Memory);
		
		static void* 	VirtualAlloc(uint64 sizeInBytes) 	{ return nullptr; }
		static bool		VirtualProtect(void* pMemory)		{ return false; }
	};
}
