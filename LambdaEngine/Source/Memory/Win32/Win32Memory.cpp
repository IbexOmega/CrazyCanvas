#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Memory/Win32/Win32Memory.h"

#include "Application/Win32/Windows.h"

namespace LambdaEngine
{
	void* Win32Memory::VirtualAlloc(uint64 sizeInBytes)
	{
		void* pAddress = ::VirtualAlloc(0, sizeInBytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		if (pAddress)
		{
			return pAddress;
		}
		else
		{
			return nullptr;
		}
	}
	
	bool Win32Memory::VirtualProtect(void* pMemory, uint64 sizeInBytes)
	{
		DWORD	dwOldProtect	= 0;
		LPVOID	lpAddress		= pMemory;

		return ::VirtualProtect(lpAddress, sizeInBytes, PAGE_NOACCESS, &dwOldProtect);
	}

	bool Win32Memory::VirtualFree(void* pMemory)
	{
		return false;
	}

	uint64 Win32Memory::GetPageSize()
	{
		SYSTEM_INFO systemInfo = { };
		::GetSystemInfo(&systemInfo);

		return uint64(systemInfo.dwPageSize);
	}
	
	uint64 Win32Memory::GetAllocationGranularity()
	{
		SYSTEM_INFO systemInfo = { };
		::GetSystemInfo(&systemInfo);

		return uint64(systemInfo.dwAllocationGranularity);
	}
}

#endif