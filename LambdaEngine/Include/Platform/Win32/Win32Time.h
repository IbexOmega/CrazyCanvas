#pragma once

#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Platform/Common/Time.h"

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

		static FORCEINLINE uint64 Nanoseconds()
		{
			LARGE_INTEGER counter = {};
			::QueryPerformanceCounter(&counter);

			return (counter.QuadPart * 1000000000UL) / s_Frequency.QuadPart;
		}

	private:
		inline static LARGE_INTEGER s_Frequency = { 0 };
	};

	typedef Win32Time PlatformTime;
}

#endif