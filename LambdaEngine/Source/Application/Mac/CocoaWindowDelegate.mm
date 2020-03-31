#ifdef LAMBDA_PLATFORM_MACOS
#ifdef __OBJC__
#include "Application/PlatformMisc.h"

#include "Application/Mac/MacApplication.h"
#include "Application/Mac/CocoaWindowDelegate.h"

@implementation CocoaWindowDelegate

- (void) windowWillClose:(NSNotification* ) notification
{
    using namespace LambdaEngine;
    MacApplication::Terminate();
}

@end

#endif
#endif
