#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#include "Platform/Common/Console.h"

#include <stdarg.h>

#include "MacConsoleWindow.h"

namespace LambdaEngine
{
    class MacConsole : public Console
    {
    public:
        DECL_STATIC_CLASS(MacConsole);
        
        static void Show();
        static void Close();
        
        static void Print(const char* pFormat, ...);
        static void PrintLine(const char* pFormat, ...);
        
        static void Clear();

        static void SetColor(EConsoleColor color);
    
    private:
        static void PrintV(const char* pFormat, va_list args);
        
        static MacConsoleWindow*    s_pConsoleWindow;
        static NSTextView*          s_pTextView;
    };

    typedef MacConsole PlatformConsole;
}

#endif
