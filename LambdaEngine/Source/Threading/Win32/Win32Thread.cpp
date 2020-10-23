#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Utilities/StringUtilities.h"

#include "Threading/Win32/Win32Thread.h"

#include "Application/Win32/Windows.h"

#include <thread>

namespace LambdaEngine
{
	/*
	* Win32Thread
	*/
	
	ThreadHandle Win32Thread::GetCurrentThreadHandle()
	{
		return reinterpret_cast<void*>(::GetCurrentThread());
	}
	
	ThreadHandle Win32Thread::GetThreadHandle(std::thread& thread)
	{
		std::thread::native_handle_type threadID = thread.native_handle();
		return reinterpret_cast<void*>(threadID);
	}

	bool Win32Thread::SetThreadName(ThreadHandle threadID, const String& name)
	{
		HANDLE	handle = static_cast<HANDLE>(threadID);
		WString	wideName = ConvertToWide(name);
		HRESULT hr = ::SetThreadDescription(handle, wideName.c_str());
		return SUCCEEDED(hr);
	}

	bool Win32Thread::SetThreadAffinity(ThreadHandle threadID, uint64 affinityMask)
	{
		HANDLE		handle = static_cast<HANDLE>(threadID);
		DWORD_PTR	result = ::SetThreadAffinityMask(handle, affinityMask);
		return result != 0;
	}
}

#endif