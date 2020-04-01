#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#include "Application/API/Misc.h"

namespace LambdaEngine
{
    class MacMisc : public Misc
    {
    public:
        DECL_STATIC_CLASS(MacMisc);

        static void MessageBox(const char* pCaption, const char* pText);

        static void OutputDebugString(const char* pFormat, ...);
        static void OutputDebugStringV(const char* pFormat, va_list args);
    };

    typedef MacMisc PlatformMisc;
}
#endif
