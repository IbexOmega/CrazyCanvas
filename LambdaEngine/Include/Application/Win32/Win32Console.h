#pragma once

#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Application/API/Console.h"

#include "Windows.h"

namespace LambdaEngine
{
	class LAMBDA_API Win32Console : public Console
	{
	public:
		static void Show();
		static void Close();

		static void Print(const char*, ...);
		static void PrintLine(const char* pFormat, ...);
        static void PrintV(const char* pFormat, va_list args);
        static void PrintLineV(const char* pFormat, va_list args);

		static void Clear();

		static void SetTitle(const char* pTitle);
		static void SetColor(EConsoleColor);

	private:
		static HANDLE s_OutputHandle;
	};

	typedef Win32Console PlatformConsole;
}

#endif