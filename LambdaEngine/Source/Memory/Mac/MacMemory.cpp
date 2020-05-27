#ifdef LAMBDA_PLATFORM_MACOS
#include "Memory/Mac/MacMemory.h"

#include <mach/mach.h>
#include <mach/mach_vm.h>

#include <unistd.h>

namespace LambdaEngine
{
	void* MacMemory::VirtualAlloc(uint64 sizeInBytes)
	{
		mach_vm_address_t 	address = 0;
		kern_return_t 		result 	= mach_vm_allocate(mach_task_self(), &address, sizeInBytes, VM_FLAGS_ANYWHERE);
		if (result == KERN_SUCCESS)
		{
			return (void*)address;
		}
		else
		{
			return nullptr;
		}
	}

	bool MacMemory::VirtualProtect(void* pMemory, uint64 sizeInBytes)
	{
		mach_vm_address_t 	address = reinterpret_cast<mach_vm_address_t>(pMemory);
		kern_return_t 		result 	= mach_vm_protect(mach_task_self(), address, (mach_vm_size_t)sizeInBytes, true, VM_PROT_NONE);
		return (result == KERN_SUCCESS);
	}

	bool MacMemory::VirtualFree(void* pMemory)
	{
		mach_vm_address_t 	address = reinterpret_cast<mach_vm_address_t>(pMemory);
		kern_return_t 		result 	= mach_vm_deallocate(mach_task_self(), address, 0);
		return (result == KERN_SUCCESS);
	}

	uint64 MacMemory::GetPageSize()
	{
		vm_size_t pageSize = 0;
		kern_return_t result = host_page_size(mach_task_self(), &pageSize);
		if (result == KERN_SUCCESS)
		{
			return pageSize;
		}
		else
		{
			return 0;
		}
	}

	uint64 MacMemory::GetAllocationGranularity()
	{
		return 0;
	}

#endif
