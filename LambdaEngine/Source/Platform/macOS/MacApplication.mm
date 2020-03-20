#ifdef LAMBDA_PLATFORM_MACOS
#include <Appkit/Appkit.h>

#include "Platform/macOS/MacApplication.h"
#include "Platform/macOS/MacAppController.h"
#include "Platform/macOS/MacWindowDelegate.h"

namespace LambdaEngine
{
    bool MacApplication::s_IsTerminating = false;
    
    bool MacApplication::PreInit()
    {
        @autoreleasepool
        {
            [NSApplication sharedApplication];
            if (NSApp != nil)
            {
                MacAppController* appDelegate = [[MacAppController alloc] init];
                [NSApp setDelegate:appDelegate];
                [NSApp activateIgnoringOtherApps:YES];
                [NSApp setPresentationOptions:NSApplicationPresentationDefault];
                [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
            }
            else
            {
                //TODO: Return error
                return false;
            }
        
            [NSApp finishLaunching];

            //TODO: Cleanup delegate and window
        
            //TODO: Other setup here

            NSUInteger  windowStyle = NSWindowStyleMaskTitled  | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;
            NSRect      windowRect  = NSMakeRect(0, 0, 800, 600);
            NSWindow*   window      = [[NSWindow alloc] initWithContentRect:windowRect styleMask:windowStyle backing:NSBackingStoreBuffered defer:NO];
            
            MacWindowDelegate* windowDelegate = [[MacWindowDelegate alloc] init];
            [window setDelegate:windowDelegate];
            [window setTitle:@"Lambda Game Engine"];
            [window makeKeyAndOrderFront:window];
        }
        
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
                if (event)
                {
                    [NSApp sendEvent:event];
                }
                [NSApp updateWindows];
                
                if (s_IsTerminating)
                {
                    return false;
                }
            }
        }
        
        return true;
    }

    void MacApplication::Terminate()
    {
        s_IsTerminating = true;
    }
}

#endif
