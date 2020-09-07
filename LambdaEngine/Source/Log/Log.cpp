#include "Log/Log.h"

#include "Threading/API/SpinLock.h"

#include "Application/API/PlatformConsole.h"
#include "Application/API/PlatformMisc.h"

#include <mutex>
#include <stdio.h>

namespace LambdaEngine
{
	bool Log::s_DebuggerOutputEnabled = false;

	/*
	* Log
	*/
	void Log::Print(ELogSeverity severity, const char* pFormat, ...)
	{
		va_list args;
		va_start(args, pFormat);
		
		PrintV(severity, pFormat, args);

		va_end(args);
	}

	void Log::PrintV(ELogSeverity severity, const char* pFormat, va_list args)
	{
		if (severity == ELogSeverity::LOG_INFO)
		{
			PlatformConsole::SetColor(EConsoleColor::COLOR_GREEN);
		}
		else if (severity == ELogSeverity::LOG_WARNING)
		{
			PlatformConsole::SetColor(EConsoleColor::COLOR_YELLOW);
		}
		else if (severity == ELogSeverity::LOG_ERROR)
		{
			PlatformConsole::SetColor(EConsoleColor::COLOR_RED);
		}

		{
			static SpinLock bufferLock;
			std::scoped_lock<SpinLock> lock(bufferLock);

			constexpr const uint32 BUFFER_SIZE = 2048;
			static char buffer[BUFFER_SIZE];

			ZERO_MEMORY(buffer, sizeof(buffer));

			vsnprintf(buffer, BUFFER_SIZE - 1, pFormat, args);
			PlatformConsole::PrintLine(buffer);
		
			if (s_DebuggerOutputEnabled)
			{
				PlatformMisc::OutputDebugString(buffer);
			}
		}

		if (severity != ELogSeverity::LOG_MESSAGE)
		{
			PlatformConsole::SetColor(EConsoleColor::COLOR_WHITE);
		}
	}

	void Log::PrintTraceError(const char* pFunction, const char* pFormat, ...)
	{
		va_list args;
		va_start(args, pFormat);

		PrintTraceErrorV(pFunction, pFormat, args);
		
		va_end(args);
	}

	void Log::PrintTraceErrorV(const char* pFunction, const char* pFormat, va_list args)
	{
		PlatformConsole::SetColor(EConsoleColor::COLOR_RED);
		PlatformConsole::Print("CRITICAL ERROR IN '%s': ", pFunction);
		PlatformConsole::SetColor(EConsoleColor::COLOR_WHITE);

		PrintV(ELogSeverity::LOG_ERROR, pFormat, args);
	}
}
