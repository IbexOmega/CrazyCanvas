#include "Log/Log.h"

#include "Platform/PlatformConsole.h"

namespace LambdaEngine
{
    void Log::Print(ELogSeverity severity, const char* pFormat, ...)
    {
        if (severity == ELogSeverity::LOG_MESSAGE)
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
        
        va_list args;
        va_start(args, pFormat);
        
        PlatformConsole::VPrintLine(pFormat, args);
        
        va_end(args);

        if (severity == ELogSeverity::LOG_MESSAGE)
        {
            PlatformConsole::SetColor(EConsoleColor::COLOR_WHITE);
        }
    }
}
