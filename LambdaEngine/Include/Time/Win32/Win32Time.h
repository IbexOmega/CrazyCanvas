#pragma once

#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Time/API/Time.h"

#include "Windows.h"

namespace LambdaEngine
{
	class Win32Time : public Time
	{
	public:
		DECL_STATIC_CLASS(Win32Time);

		static FORCEINLINE void PreInit()
		{
			::QueryPerformanceFrequency(&s_Frequency);
		}

		static FORCEINLINE uint64 GetPerformanceCounter()
		{
			LARGE_INTEGER counter = {};
			::QueryPerformanceCounter(&counter);

			return uint64(counter.QuadPart);
		}

		static FORCEINLINE uint64 GetPerformanceFrequency()
		{
			return uint64(s_Frequency.QuadPart);
		}

	private:
		inline static LARGE_INTEGER s_Frequency = { 1 };
	};

	typedef Win32Time PlatformTime;
}

#endif