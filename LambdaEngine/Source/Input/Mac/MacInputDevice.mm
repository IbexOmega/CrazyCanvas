#ifdef LAMBDA_PLATFORM_MACOS
#include "Log/Log.h"

#include "Input/Mac/MacInputDevice.h"
#include "Input/Mac/MacInputCodeTable.h"

#include "Application/Mac/MacApplication.h"
#include "Application/Mac/MacScopedPool.h"

#include <AppKit/AppKit.h>

namespace LambdaEngine
{
    void MacInputDevice::HandleEvent(const MacEvent* pEvent)
    {
        NSEvent* event = pEvent->pEvent;
        if (event)
        {
            NSEventType eventType = [event type];
            switch(eventType)
            {
                case NSEventTypeKeyUp:
                {
                    const uint16  macKey = [event keyCode];
                    const EKey    key    = MacInputCodeTable::GetKey(macKey);
                    
                    OnKeyReleased(key);
                    break;
                }
                    
                case NSEventTypeKeyDown:
                {
                    const uint16  macKey = [event keyCode];
                    const EKey    key    = MacInputCodeTable::GetKey(macKey);
                
                    const uint32 modifierFlags  = [event modifierFlags];
                    const uint32 modifiers      = MacInputCodeTable::GetModiferMask(modifierFlags);
                    
                    OnKeyPressed(key, modifiers, [event isARepeat]);
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
                    
                    const NSUInteger modifierFlags = [event modifierFlags];
                    const uint32     modifierMask  = MacInputCodeTable::GetModiferMask(modifierFlags);
                    
                    if (button != EMouseButton::MOUSE_BUTTON_UNKNOWN)
                    {
                        OnMouseButtonPressed(button, modifierMask);
                    }
                    
                    break;
                }
                
                case NSEventTypeLeftMouseDragged:
                case NSEventTypeOtherMouseDragged:
                case NSEventTypeRightMouseDragged:
                case NSEventTypeMouseMoved:
                {
                    const NSPoint mousePosition = [event locationInWindow];
                    
                    int32 x = 0;
                    int32 y = 0;
                    if (pEvent->pEventWindow)
                    {
                        const NSRect contentRect = [pEvent->pEventWindow frame];
                        x = int32(mousePosition.x);
                        y = int32(contentRect.size.height - mousePosition.y);
                    }
                    else
                    {
                        x = int32(mousePosition.x);
                        y = int32(mousePosition.y);
                    }
     
                    OnMouseMoved(x, y);
                    break;
                }
                    
                case NSEventTypeScrollWheel:
                {
                    CGFloat scrollDeltaX = [event scrollingDeltaX];
                    CGFloat scrollDeltaY = [event scrollingDeltaY];
                    if ([event hasPreciseScrollingDeltas])
                    {
                        scrollDeltaX *= 0.1;
                        scrollDeltaY *= 0.1;
                    }
                    
                    OnMouseScrolled(int32(scrollDeltaX), int32(scrollDeltaY));
                    break;
                }
                    
                default:
                {
                    break;
                }
            }
        }
        else if (pEvent->pKeyTypedText)
        {
            NSString*   text    = pEvent->pKeyTypedText;
            NSUInteger  count   = [text length];
            for (NSUInteger i = 0; i < count; i++)
            {
                // Equal to unsigned short
                const unichar codepoint = [text characterAtIndex:i];
                if ((codepoint & 0xff00) != 0xf700)
                {
                    OnKeyTyped(uint32(codepoint));
                }
            }
        }
    }
}

#endif
