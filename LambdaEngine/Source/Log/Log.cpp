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

	void Log::PrintV(ELogSeverity severity, const char* pFormat, va_list vaArgs)
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

			static String buffer;
			static String lastMessage;

			// Check length of formated string and resize buffer
			va_list vaCopy;
			va_copy(vaCopy, vaArgs);
			int length = vsnprintf(nullptr, 0, pFormat, vaCopy);
			va_end(vaCopy);

			buffer.resize(length);
			// Since we reserve 1 more char than length this should be safe to do
			vsnprintf(buffer.data(), buffer.size() + 1, pFormat, vaArgs);

			// Print message
			if (s_DebuggerOutputEnabled)
			{
				PlatformMisc::OutputDebugString(buffer.c_str());
			}

			// Check if last message is the same
			static ELogSeverity lastSeverity;
			static uint32 messageCount = 1;
			if (lastMessage != buffer || lastSeverity != severity)
			{
				lastMessage = buffer;
				lastSeverity = severity;
				messageCount = 1;
			}
			else
			{
				messageCount++;
				buffer += " (x" + std::to_string(messageCount) + " Times)";

				PlatformConsole::ClearLastLine();
			}

			PlatformConsole::PrintLine(buffer.c_str());
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

	void Log::PrintTraceErrorV(const char* pFunction, const char* pFormat, va_list vaArgs)
	{
		PlatformConsole::SetColor(EConsoleColor::COLOR_RED);
		PlatformConsole::Print("CRITICAL ERROR IN '%s': ", pFunction);
		PlatformConsole::SetColor(EConsoleColor::COLOR_WHITE);

		PrintV(ELogSeverity::LOG_ERROR, pFormat, vaArgs);
	}
}
