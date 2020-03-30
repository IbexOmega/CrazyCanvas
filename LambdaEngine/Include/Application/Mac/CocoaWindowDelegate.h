#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#ifdef __OBJC__

#include <Appkit/Appkit.h>

@interface CocoaWindowDelegate : NSObject<NSWindowDelegate>

- (void) windowWillClose:(NSNotification* ) notification;

@end

#else

class CocoaWindowDelegate;

#endif
#endif


