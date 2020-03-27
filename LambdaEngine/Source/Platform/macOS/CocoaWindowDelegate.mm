#ifdef LAMBDA_PLATFORM_MACOS
#include "Platform/PlatformMisc.h"

#include "Platform/macOS/MacApplication.h"
#include "Platform/macOS/CocoaWindowDelegate.h"

@implementation CocoaWindowDelegate

- (void) windowWillClose:(NSNotification* ) notification
{
    using namespace LambdaEngine;
    MacApplication::Terminate();
}

@end

#endif
