#if defined(LAMBDA_PLATFORM_MACOS) && defined(__OBJC__)
#include "Application/API/PlatformMisc.h"
#include "Application/API/PlatformApplication.h"

#include "Application/Mac/CocoaAppDelegate.h"

@implementation CocoaAppDelegate

- (BOOL) applicationShouldTerminateAfterLastWindowClosed:(NSApplication* ) sender
{
    using namespace LambdaEngine;
    
    PlatformMisc::OutputDebugString("Terminate After Last Window Closed");
    return YES;
}

- (void) applicationWillTerminate:(NSNotification* ) notification
{
    using namespace LambdaEngine;
    
    MacEvent storedEvent = { };
    storedEvent.pNotification = [notification retain];
        
    MacApplication::Get()->StoreEvent(storedEvent);
}

@end

#endif
