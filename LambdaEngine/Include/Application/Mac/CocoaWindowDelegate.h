#pragma once

#if defined(LAMBDA_PLATFORM_MACOS) && defined(__OBJC__)

#include <Appkit/Appkit.h>

@interface CocoaWindowDelegate : NSObject<NSWindowDelegate>

- (void) windowWillClose:(NSNotification* ) notification;

@end

#else

class CocoaWindowDelegate;

#endif


