#if defined(LAMBDA_PLATFORM_MACOS) && defined(__OBJC__)
#include "Application/API/PlatformMisc.h"

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
