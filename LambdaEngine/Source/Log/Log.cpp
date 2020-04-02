#include "Log/Log.h"

#include "Application/API/PlatformConsole.h"
#include "Application/API/PlatformMisc.h"

namespace LambdaEngine
{
    bool Log::s_DebuggerOutputEnabled = false;

    void Log::Print(ELogSeverity severity, const char* pFormat, ...)
    {
        va_list args;
        va_start(args, pFormat);
        Log::VPrint(severity, pFormat, args);
        va_end(args);
    }

    void Log::VPrint(ELogSeverity severity, const char* pFormat, va_list args)
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

        va_list argsCopy;
        va_copy(argsCopy, args);
        PlatformConsole::VPrintLine(pFormat, argsCopy);
        va_end(argsCopy);
        
        if (s_DebuggerOutputEnabled)
        {
            PlatformMisc::OutputDebugStringV(pFormat, args);
        }

		if (severity != ELogSeverity::LOG_MESSAGE)
		{
			PlatformConsole::SetColor(EConsoleColor::COLOR_WHITE);
		}
    }

    void Log::PrintTraceError(const char* pFunction, const char* pFormat, ...)
    {
        PlatformConsole::SetColor(EConsoleColor::COLOR_RED);
        PlatformConsole::Print("Critical Error in '%s': ", pFunction);
        PlatformConsole::SetColor(EConsoleColor::COLOR_WHITE);
        
        va_list args;
        va_start(args, pFormat);
        Log::VPrint(ELogSeverity::LOG_ERROR, pFormat, args);
        va_end(args);
    }
}
