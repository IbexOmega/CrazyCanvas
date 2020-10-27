#pragma once
#include "LambdaEngine.h"

namespace LambdaEngine
{
	/*
	* GenericThread
	*/

	typedef void* ThreadHandle;

	class GenericThread
	{
	public:
		static ThreadHandle GetCurrentThreadHandle()
		{
			return nullptr;
		}

		static ThreadHandle GetThreadHandle(std::thread& thread)
		{
			UNREFERENCED_VARIABLE(thread);
			return nullptr;
		}

		static bool SetThreadName(ThreadHandle threadID, const String& name)
		{
			UNREFERENCED_VARIABLE(threadID);
			UNREFERENCED_VARIABLE(name);
		}

		/*
		* The affinityMask represents a bit-mask of what cores the threads are allowed to run on
		*/
		static bool SetThreadAffinity(ThreadHandle threadID, uint64 affinityMask)
		{
			UNREFERENCED_VARIABLE(threadID);
			UNREFERENCED_VARIABLE(affinityMask);
		}
	};
}