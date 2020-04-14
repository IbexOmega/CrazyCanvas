#ifdef LAMBDA_PLATFORM_MACOS
#include "Input/Mac/MacInputDevice.h"
#include "Input/Mac/MacInputCodeTable.h"

#include "Application/Mac/MacScopedPool.h"

#include <AppKit/AppKit.h>

namespace LambdaEngine
{
    void MacInputDevice::HandleEvent(NSEvent* event)
    {
        NSEventType eventType = [event type];
        switch(eventType)
        {
            case NSEventTypeKeyUp:
            {
                const uint16  macKey = [event keyCode];
                const EKey    key    = MacInputCodeTable::GetKey(macKey);
                
                OnKeyUp(key);
                break;
            }
                
            case NSEventTypeKeyDown:
            {
                const uint16  macKey = [event keyCode];
                const EKey    key    = MacInputCodeTable::GetKey(macKey);
                
                OnKeyDown(key);
                break;
            }
            
            case NSEventTypeLeftMouseUp:
            case NSEventTypeRightMouseUp:
            case NSEventTypeOtherMouseUp:
            {
                const NSInteger         macButton   = [event buttonNumber];
                const EMouseButton      button      = MacInputCodeTable::GetMouseButton(int32(macButton));
                
                if (button != EMouseButton::MOUSE_BUTTON_UNKNOWN)
                {
                    OnMouseButtonReleased(button);
                }
                
                break;
            }
            
            case NSEventTypeLeftMouseDown:
            case NSEventTypeRightMouseDown:
            case NSEventTypeOtherMouseDown:
            {
                const NSInteger       macButton   = [event buttonNumber];
                const EMouseButton    button      = MacInputCodeTable::GetMouseButton(int32(macButton));
                
                if (button != EMouseButton::MOUSE_BUTTON_UNKNOWN)
                {
                    OnMouseButtonPressed(button);
                }
                
                break;
            }
                
            case NSEventTypeMouseMoved:
            {
                const NSPoint   mousePosition   = [event locationInWindow];
                NSWindow*       window          = [event window];
                
                int32 x = 0;
                int32 y = 0;
                if (window)
                {
                    const NSRect contentRect = [window frame];
                    x = int32(mousePosition.x);
                    y = int32(contentRect.size.height - mousePosition.y);
                }
                else
                {
                    x = int32(mousePosition.x);
                    y = int32(mousePosition.y);
                }
 
                OnMouseMove(x, y);
                break;
            }
                
            case NSEventTypeScrollWheel:
            {
                const CGFloat scrollDelta = [event scrollingDeltaX];
                
                OnMouseScrolled(int32(scrollDelta));
                break;
            }
                
            default:
            {
                break;
            }
        }
    }
}

#endif
