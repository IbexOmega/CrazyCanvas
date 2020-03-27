#ifdef LAMBDA_PLATFORM_MACOS
#include "Platform/macOS/CocoaWindow.h"

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
