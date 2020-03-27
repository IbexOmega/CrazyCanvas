#ifdef LAMBDA_PLATFORM_MACOS
#include "Log/Log.h"

#include "Platform/macOS/MacInputDevice.h"

#include <AppKit/AppKit.h>

namespace LambdaEngine
{
    void MacInputDevice::HandleEvent(NSEvent* event)
    {
        NSEventType eventType = [event type];
        switch(eventType)
        {
            case NSMouseMoved:
            {
                LOG_MESSAGE("Mouse moved");
                break;
            }
        }
    }
}

#endif
