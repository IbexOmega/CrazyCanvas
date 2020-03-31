#ifdef LAMBDA_PLATFORM_MACOS
#ifdef __OBJC__
#include "Application/Mac/CocoaConsoleWindow.h"

@implementation CocoaConsoleWindow

-(BOOL) windowShouldClose:(NSWindow* ) sender
{
    [sender release];
    return YES;
}

@end

#endif
#endif
