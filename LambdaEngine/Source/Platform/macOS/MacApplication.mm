#ifdef LAMBDA_PLATFORM_MACOS
#include "Platform/macOS/MacApplication.h"
#include "Platform/macOS/MacAppController.h"
#include "Platform/macOS/MacWindowDelegate.h"

namespace LambdaEngine
{
    MacWindow           MacApplication::s_Window            = MacWindow();
    MacAppController*   MacApplication::s_pAppDelegate      = nullptr;
    bool                MacApplication::s_IsTerminating     = false;
    
    bool MacApplication::PreInit()
    {
        [NSApplication sharedApplication];
        if (NSApp != nil)
        {
            s_pAppDelegate = [[MacAppController alloc] init];
            [NSApp setDelegate:s_pAppDelegate];
            [NSApp activateIgnoringOtherApps:YES];
            [NSApp setPresentationOptions:NSApplicationPresentationDefault];
            [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        }
        else
        {
            //TODO: Log error
            return false;
        }
    
        [NSApp finishLaunching];
    
        //TODO: Other setup here

        if (!s_Window.Init(800, 600))
        {
            return false;
        }
        
        s_Window.Show();
        
        return true;
    }

    bool MacApplication::Tick()
    {
        @autoreleasepool
        {
            NSEvent* event = nil;
            while (true)
            {
                event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES];
                if (event == nil)
                {
                    break;
                }
                
                [NSApp sendEvent:event];
                [NSApp updateWindows];
                
                if (s_IsTerminating)
                {
                    return false;
                }
            }
        }
        
        return true;
    }

    bool MacApplication::PostRelease()
    {
        [s_pAppDelegate release];
        
        s_Window.Release();
        return true;
    }

    void MacApplication::Terminate()
    {
        s_IsTerminating = true;
    }
}

#endif
