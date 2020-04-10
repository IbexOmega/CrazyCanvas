#ifdef LAMBDA_PLATFORM_MACOS
#ifdef __OBJC__
#include "Application/Mac/CocoaConsoleWindow.h"
#include "Application/Mac/MacScopedPool.h"

@implementation CocoaConsoleWindow

-(BOOL) windowShouldClose:(NSWindow* ) sender
{
    SCOPED_AUTORELEASE_POOL();
    
    [sender release];
    return YES;
}

@end

#endif
#endif
