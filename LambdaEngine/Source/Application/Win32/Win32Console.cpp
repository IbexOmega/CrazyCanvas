#ifdef LAMBDA_PLATFORM_WINDOWS
#include <stdio.h>
#include <stdarg.h>
#include <mutex>

#include "Threading/API/SpinLock.h"

#include "Application/Win32/Win32Console.h"

namespace LambdaEngine
{
	//Locks the console outputhandle
	SpinLock g_ConsoleLock;

	HANDLE Win32Console::s_OutputHandle = 0;

	void Win32Console::Show()
	{
		std::scoped_lock<SpinLock> lock(g_ConsoleLock);

		if (AllocConsole())
		{
			s_OutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
			SetConsoleTitleA("Lambda Engine Debug Console");
		}
	}

	void Win32Console::Close()
	{
		std::scoped_lock<SpinLock> lock(g_ConsoleLock);

		if (s_OutputHandle)
		{
			FreeConsole();
			s_OutputHandle = 0;
		}
	}

	void Win32Console::Print(const char* pMessage, ...)
	{
		va_list args;
		va_start(args, pMessage);

		VPrint(pMessage, args);

		va_end(args);
	}

	void Win32Console::PrintLine(const char* pMessage, ...)
	{
		va_list args;
		va_start(args, pMessage);

		VPrintLine(pMessage, args);

		va_end(args);
	}

	void Win32Console::VPrint(const char* pMessage, va_list args)
	{
		std::scoped_lock<SpinLock> lock(g_ConsoleLock);

		if (s_OutputHandle)
		{
			constexpr uint32 BUFFER_SIZE = 1024;
			static char buffer[BUFFER_SIZE];
			ZERO_MEMORY(buffer, BUFFER_SIZE);

			int numChars = vsprintf_s(buffer, BUFFER_SIZE, pMessage, args);
			if (numChars > 0)
			{
				WriteConsoleA(s_OutputHandle, buffer, numChars, 0, NULL);
			}
		}
	}

	void Win32Console::VPrintLine(const char* pMessage, va_list args)
	{
		std::scoped_lock<SpinLock> lock(g_ConsoleLock);

		if (s_OutputHandle)
		{
			constexpr uint32 BUFFER_SIZE = 2048;
			static char buffer[BUFFER_SIZE];
			ZERO_MEMORY(buffer, BUFFER_SIZE);

			int numChars = vsprintf_s(buffer, BUFFER_SIZE - 1, pMessage, args);
			if (numChars > 0)
			{
				buffer[numChars] = '\n';
				WriteConsoleA(s_OutputHandle, buffer, numChars + 1, 0, NULL);
			}
		}
	}

	void Win32Console::Clear()
	{
		std::scoped_lock<SpinLock> lock(g_ConsoleLock);

		if (s_OutputHandle)
		{
			CONSOLE_SCREEN_BUFFER_INFO csbi;

			if (!GetConsoleScreenBufferInfo(s_OutputHandle, &csbi))
			{
				return;
			}

			COORD dst = { 0, -csbi.dwSize.Y };
			CHAR_INFO fillInfo = { '\0', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE };
			ScrollConsoleScreenBufferA(s_OutputHandle, &csbi.srWindow, nullptr, dst, &fillInfo);

			COORD cursorPos = { 0, 0 };
			SetConsoleCursorPosition(s_OutputHandle, cursorPos);
		}
	}

	void Win32Console::SetTitle(const char* pTitle)
	{
		std::scoped_lock<SpinLock> lock(g_ConsoleLock);

		if (s_OutputHandle)
		{
			::SetConsoleTitleA(pTitle);
		}
	}

	void Win32Console::SetColor(EConsoleColor color)
	{
		std::scoped_lock<SpinLock> lock(g_ConsoleLock);

		if (s_OutputHandle)
		{
			WORD wColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

			switch (color)
			{
			case EConsoleColor::COLOR_RED:
				wColor = FOREGROUND_RED | FOREGROUND_INTENSITY;
				break;

			case EConsoleColor::COLOR_GREEN:
				wColor = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
				break;

			case EConsoleColor::COLOR_YELLOW:
				wColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
				break;

			case EConsoleColor::COLOR_WHITE:
				break;
			}

			SetConsoleTextAttribute(s_OutputHandle, wColor);
		}
	}
}

#endif