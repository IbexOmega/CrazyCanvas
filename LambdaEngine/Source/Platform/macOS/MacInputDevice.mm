#ifdef LAMBDA_PLATFORM_MACOS
#include "Log/Log.h"

#include "Platform/macOS/MacInputDevice.h"

namespace LambdaEngine
{
    void MacInputDevice::HandleEvent(NSEvent* event)
    {
        LOG_MESSAGE("Got an event");
    }
}

#endif
