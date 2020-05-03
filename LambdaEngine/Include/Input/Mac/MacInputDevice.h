#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#include "Input/API/InputDeviceBase.h"

#include "Application/Mac/IMacMessageHandler.h"

namespace LambdaEngine
{
    class MacInputDevice : public InputDeviceBase, public IMacMessageHandler
    {
    public:
        MacInputDevice()    = default;
        ~MacInputDevice()   = default;
        
        virtual void HandleEvent(const MacEvent* pEvent) override final;
    };
}

#endif
