#ifdef LAMBDA_PLATFORM_MACOS
#include "Platform/PlatformMisc.h"

#include "Platform/macOS/MacApplication.h"
#include "Platform/macOS/MacWindowDelegate.h"

@implementation MacWindowDelegate

- (void) windowWillClose:(NSNotification* ) notification
{
    using namespace LambdaEngine;
    
    PlatformMisc::OutputDebugString("Window Will Close");
    MacApplication::Terminate();
}

@end

#endif
