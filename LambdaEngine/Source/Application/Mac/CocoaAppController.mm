#if defined(LAMBDA_PLATFORM_MACOS) && defined(__OBJC__)
#include "Application/API/PlatformMisc.h"

#include "Application/Mac/CocoaAppController.h"

@implementation CocoaAppController

-(BOOL) applicationShouldTerminateAfterLastWindowClosed:(NSApplication* ) sender
{
    using namespace LambdaEngine;
    
    PlatformMisc::OutputDebugString("Terminate After Last Window Closed");
    return YES;
}

@end

#endif
