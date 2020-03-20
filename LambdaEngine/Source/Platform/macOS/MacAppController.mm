#ifdef LAMBDA_PLATFORM_MACOS
#include "Platform/macOS/MacAppController.h"

@implementation MacAppController

-(BOOL) applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender
{
    NSLog(@"Terminate After Last Window Closed");
    return YES;
}

@end

#endif
