#ifdef LAMBDA_PLATFORM_MACOS
#include "Application/API/PlatformMisc.h"

#include "Application/Mac/MacAppController.h"

@implementation MacAppController

-(BOOL) applicationShouldTerminateAfterLastWindowClosed:(NSApplication* ) sender
{
    using namespace LambdaEngine;
    
    PlatformMisc::OutputDebugString("Terminate After Last Window Closed");
    return YES;
}

@end

#endif
