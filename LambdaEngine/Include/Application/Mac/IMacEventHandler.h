#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#include "LambdaEngine.h"

#ifdef __OBJC__
@class NSEvent;
#else
class NSEvent;
#endif

namespace LambdaEngine
{
    class IMacEventHandler
    {
    public:
        DECL_INTERFACE(IMacEventHandler);
        
        /*
        * Handles events sent to the application
        *   event - The event sent from the application.
        */
        virtual void HandleEvent(NSEvent* event) = 0;
    };
}

#endif
