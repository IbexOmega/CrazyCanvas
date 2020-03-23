#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#ifdef __OBJC__

#include <Appkit/Appkit.h>
#include <Foundation/Foundation.h>

@interface MacWindowDelegate : NSObject<NSWindowDelegate>

- (void) windowWillClose:(NSNotification* ) notification;

@end

#else

class MacWindowDelegate;

#endif
#endif


