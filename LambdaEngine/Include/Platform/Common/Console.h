#pragma once
#include "LambdaEngine.h"

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
        DECL_STATIC_CLASS(Console);
        
        static void Show()  { }
        static void Close() { }
        
        static void Print(const char*, ...)     { }
        static void PrintLine(const char*, ...) { }
        
        static void Clear() { }
        
        static void SetColor(EConsoleColor) { }
    };
}
