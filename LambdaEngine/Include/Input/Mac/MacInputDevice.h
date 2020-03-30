#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#include "Input/API/InputDevice.h"

namespace LambdaEngine
{
    class MacInputDevice : public InputDevice
    {
    public:
        MacInputDevice()    = default;
        ~MacInputDevice()   = default;

        virtual void HandleEvent(NSEvent* event) override;
    };
}


#endif
