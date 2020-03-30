#include "Log/Log.h"

#include "Application/PlatformConsole.h"

namespace LambdaEngine
{
    void Log::Print(ELogSeverity severity, const char* pFormat, ...)
    {
        if (severity == ELogSeverity::MESSAGE)
        {
            PlatformConsole::SetColor(EConsoleColor::COLOR_GREEN);
        }
        else if (severity == ELogSeverity::WARNING)
        {
            PlatformConsole::SetColor(EConsoleColor::COLOR_YELLOW);
        }
        else if (severity == ELogSeverity::ERROR)
        {
            PlatformConsole::SetColor(EConsoleColor::COLOR_RED);
        }
        
        va_list args;
        va_start(args, pFormat);
        
        PlatformConsole::VPrintLine(pFormat, args);
        
        va_end(args);

        if (severity != ELogSeverity::MESSAGE)
        {
            PlatformConsole::SetColor(EConsoleColor::COLOR_WHITE);
        }
    }
}
