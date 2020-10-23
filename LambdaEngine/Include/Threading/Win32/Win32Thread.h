#pragma once
#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Threading/API/GenericThread.h"

namespace LambdaEngine
{
	/*
	* Win32Thread
	*/

	class Win32Thread : public GenericThread
	{
	public:
		static ThreadHandle GetCurrentThreadHandle();
		static ThreadHandle GetThreadHandle(std::thread& thread);

		static bool SetThreadName(ThreadHandle threadID, const String& name);
		static bool SetThreadAffinity(ThreadHandle threadID, uint64 affinityMask);
	};

	typedef Win32Thread PlatformThread;
}

#endif