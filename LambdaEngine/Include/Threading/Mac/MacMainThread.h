#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#include "LambdaEngine.h"

#include <CoreFoundation/CoreFoundation.h>

#ifdef __OBJC__
#include <Foundation/Foundation.h>
#else
class NSString;
#endif

namespace LambdaEngine
{
    class MacRunLoopSource;

    class MacMainThread
    {
    public:
        DECL_STATIC_CLASS(MacMainThread);
        
        static void PreInit();
        static void PostRelease();
        
        static void Tick();
        
        static void MakeCall(dispatch_block_t block, bool waitUntilFinished);
        
    private:
        static MacRunLoopSource* s_pMainThread;
    };
}

#endif
