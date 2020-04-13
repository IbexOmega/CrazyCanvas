#if defined(LAMBDA_PLATFORM_MACOS) && defined(__OBJC__)
#include "Application/Mac/CocoaWindow.h"

@implementation CocoaWindow

- (BOOL)canBecomeKeyWindow
{
    return YES;
}

- (BOOL)canBecomeMainWindow
{
    return YES;
}

- (BOOL)acceptsMouseMovedEvents
{
    return YES;
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

@end

#endif
