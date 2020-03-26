#pragma once
#include "LambdaEngine.h"

#include <stdarg.h>

namespace LambdaEngine
{
    enum class EConsoleColor : uint8
    {
        COLOR_RED     = 0,
        COLOR_GREEN   = 1,
        COLOR_YELLOW  = 2,
        COLOR_WHITE   = 3
    };

    class Console
    {
    public:
        Console()   = default;
        ~Console()  = default;
        
        DECL_REMOVE_COPY(Console);
        DECL_REMOVE_MOVE(Console);
        
        static void Show()  { }
        static void Close() { }
        
        static void Print(const char*, ...)     { }
        static void PrintLine(const char*, ...) { }
        static void VPrint(const char*, va_list) { }
        static void VPrintLine(const char*, va_list) { }
        
        static void Clear() { }
        
        static void SetColor(EConsoleColor) { }
    };
}
