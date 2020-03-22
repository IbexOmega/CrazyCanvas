#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#include "Platform/Common/Application.h"

#include "MacWindow.h"
#include "MacAppController.h"

namespace LambdaEngine
{
    class LAMBDA_API MacApplication : public Application
    {
    public:
        DECL_STATIC_CLASS(MacApplication);

        static bool PreInit();
        static bool PostRelease();
        
        static bool Tick();
        
        static void Terminate();
        
    private:
        static MacWindow            s_Window;
        static MacAppController*    s_AppDelegate;
        static bool                 s_IsTerminating;
    };

    typedef MacApplication PlatformApplication;
}

#endif
