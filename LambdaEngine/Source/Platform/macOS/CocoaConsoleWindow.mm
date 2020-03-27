#ifdef LAMBDA_PLATFORM_MACOS
#include "Platform/macOS/CocoaConsoleWindow.h"

@implementation CocoaConsoleWindow

-(BOOL) windowShouldClose:(NSWindow* ) sender
{
    [sender release];
    return YES;
}

@end

#endif
