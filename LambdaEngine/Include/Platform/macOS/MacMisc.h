#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#include "Platform/Common/Misc.h"

namespace LambdaEngine
{
    class LAMBDA_API MacMisc : public Misc
    {
    public:
        DECL_STATIC_CLASS(MacMisc);

        static void MessageBox(const char* pCaption, const char* pText);
        static void OutputDebugString(const char* pDebugString, ...);
    };

    typedef MacMisc PlatformMisc;
}
#endif
