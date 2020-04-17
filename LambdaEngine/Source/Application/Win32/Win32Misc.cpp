#ifdef LAMBDA_PLATFORM_WINDOWS
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
		va_list args;
		va_start(args, pFormat);
		OutputDebugStringV(pFormat, args);
		va_end(args);
	}

	void Win32Misc::OutputDebugStringV(const char* pFormat, va_list args)
	{
		constexpr uint32 BUFFER_SIZE = 2048;
		static char buffer[BUFFER_SIZE];

		ZERO_MEMORY(buffer, BUFFER_SIZE);

		vsprintf_s(buffer, BUFFER_SIZE - 1, pFormat, args);
		OutputDebugStringA(buffer);
		OutputDebugStringA("\n");
	}
}

#endif