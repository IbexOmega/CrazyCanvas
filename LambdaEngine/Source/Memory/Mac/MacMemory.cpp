#ifdef LAMBDA_PLATFORM_MACOS
#include "Memory/Mac/MacMemory.h"

#include <mach/mach.h>
#include <mach/mach_vm.h>

namespace LambdaEngine
{
	void* MacMemory::VirtualAlloc(uint64 sizeInBytes)
	{
		mach_vm_address_t address = 0;
		kern_return_t result = mach_vm_allocate(mach_task_self(), &address, sizeInBytes, VM_FLAGS_ANYWHERE);
		if (result == KERN_SUCCESS)
		{
			return (void*)address;
		}
		else
		{
			return nullptr;
		}
	}

	bool MacMemory::VirtualProtect(void* pMemory)
	{
		return false;
	}
}

#endif
