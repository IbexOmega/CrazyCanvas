#ifdef LAMBDA_PLATFORM_WINDOWS
#include <stdio.h>
#include <mutex>

#include "Threading/API/SpinLock.h"

#include "Application/Win32/Win32Misc.h"
#include "Application/Win32/Windows.h"

namespace LambdaEngine
{
	/*
	* Win32Misc
	*/
	void Win32Misc::MessageBox(const char* pCaption, const char* pText)
	{
		::MessageBoxA(0, pText, pCaption, MB_ICONERROR | MB_OK);
	}

	void Win32Misc::OutputDebugString(const char* pFormat, ...)
	{
		va_list args;
		va_start(args, pFormat);
		
		OutputDebugStringV(pFormat, args);
		
		va_end(args);
	}

	void Win32Misc::OutputDebugStringV(const char* pFormat, va_list args)
	{
		static SpinLock bufferlock;
		std::scoped_lock<SpinLock> lock(bufferlock);

		constexpr uint32 BUFFER_SIZE = 2048;
		static char buffer[BUFFER_SIZE];

		int32 written = vsprintf_s(buffer, BUFFER_SIZE - 2, pFormat, args);
		if (written > 0)
		{
			buffer[written]		= '\n';
			buffer[written + 1] = 0;
			::OutputDebugStringA(buffer);
		}
	}
}

#endif