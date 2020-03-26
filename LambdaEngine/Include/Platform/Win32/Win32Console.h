#pragma once

#ifdef LAMBDA_PLATFORM_WINDOWS

#include "Platform/Common/Console.h"

#include "Platform/Win32/Windows.h"

namespace LambdaEngine
{
	class LAMBDA_API Win32Console : public Console
	{
	public:
		static void Show();
		static void Close();

		static void Print(const char*, ...);
		static void PrintLine(const char*, ...);

		static void Clear();

		static void SetColor(EConsoleColor);

	private:
		static HANDLE s_OutputHandle;

	};

	typedef Win32Console PlatformConsole;
}

#endif