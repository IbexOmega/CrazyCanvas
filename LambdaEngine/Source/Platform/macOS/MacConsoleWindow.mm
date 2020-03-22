#ifdef LAMBDA_PLATFORM_MACOS
#include "Platform/macOS/MacConsoleWindow.h"

@implementation MacConsoleWindow

-(BOOL) windowShouldClose:(NSWindow* ) sender
{
    [sender release];
    return YES;
}

@end

#endif
