#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#include "LambdaEngine.h"

namespace LambdaEngine
{
    struct MacEvent;

    class IMacMessageHandler
    {
    public:
        DECL_INTERFACE(IMacMessageHandler);
        
        /*
        * Handles events sent to the application
        *   event - The event sent from the application.
        */
        virtual void HandleEvent(const MacEvent* pEvent) = 0;
    };
}

#endif
