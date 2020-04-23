#if defined(LAMBDA_PLATFORM_MACOS) && defined(__OBJC__)
#include "Log/Log.h"

#include "Application/Mac/CocoaWindow.h"
#include "Application/Mac/MacApplication.h"

#include "Input/Mac/MacInputCodeTable.h"

@implementation CocoaWindow

- (BOOL) canBecomeKeyWindow
{
    return YES;
}

- (BOOL) canBecomeMainWindow
{
    return YES;
}

- (BOOL) acceptsMouseMovedEvents
{
    return YES;
}

@end

#endif
