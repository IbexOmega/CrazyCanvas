#ifdef LAMBDA_PLATFORM_WINDOWS
#include <stdarg.h>
#include <stdio.h>

#include "Application/Win32/Win32Misc.h"
#include "Application/Win32/Windows.h"

namespace LambdaEngine
{
	void Win32Misc::MessageBox(const char* pCaption, const char* pText)
	{
		::MessageBoxA(0, pText, pCaption, MB_ICONERROR | MB_OK);
	}

	void Win32Misc::OutputDebugString(const char* pFormat, ...)
	{
		constexpr uint32 BUFFER_SIZE = 1024;
		static char buffer[BUFFER_SIZE];
		
		ZERO_MEMORY(buffer, BUFFER_SIZE);

		va_list args;
		va_start(args, pFormat);
		vsprintf_s(buffer, BUFFER_SIZE - 1, pFormat, args);
		va_end(args);

		OutputDebugStringA(buffer);
	}
}

#endif