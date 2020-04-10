#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#include "LambdaEngine.h"

#include <CoreFoundation/CoreFoundation.h>

namespace LambdaEngine
{
    class MacRunLoopSource;

    class MacMainThread
    {
    public:
        DECL_STATIC_CLASS(MacMainThread);
        
        static void PreInit();
        static void PostRelease();
        
        static void MakeCall(dispatch_block_t block);
        
    private:
        static MacRunLoopSource* s_pMainThread;
    };
}

#endif
