#ifdef LAMBDA_PLATFORM_MACOS
#include "Platform/PlatformMisc.h"

#include "Platform/macOS/MacAppController.h"

@implementation MacAppController

-(BOOL) applicationShouldTerminateAfterLastWindowClosed:(NSApplication* ) sender
{
    using namespace LambdaEngine;
    
    PlatformMisc::OutputDebugString("Terminate After Last Window Closed");
    return YES;
}

@end

#endif
