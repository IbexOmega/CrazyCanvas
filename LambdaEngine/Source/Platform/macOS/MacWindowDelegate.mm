#ifdef LAMBDA_PLATFORM_MACOS
#include "Platform/macOS/MacApplication.h"
#include "Platform/macOS/MacWindowDelegate.h"

@implementation MacWindowDelegate

- (void) windowWillClose:(NSNotification *)notification
{
    using namespace LambdaEngine;
    
    NSLog(@"Window Will Close");
    MacApplication::Terminate();
}

@end

#endif
