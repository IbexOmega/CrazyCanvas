#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#include "Application/API/Console.h"

#ifdef __OBJC__
@class CocoaConsoleWindow;
#else
class CocoaConsoleWindow;
#endif

namespace LambdaEngine
{
    class MacConsole : public Console
    {
    public:
        static void Show();
        static void Close();
        
        static void Print(const char* pFormat, ...);
        static void PrintLine(const char* pFormat, ...);
        static void VPrint(const char* pFormat, va_list args);
        static void VPrintLine(const char* pFormat, va_list args);
        
        static void Clear();

        static void SetTitle(const char* pTitle);
        static void SetColor(EConsoleColor color);
        
    private:
        static CocoaConsoleWindow* s_pConsoleWindow;
    };

    typedef MacConsole PlatformConsole;
}

#endif
