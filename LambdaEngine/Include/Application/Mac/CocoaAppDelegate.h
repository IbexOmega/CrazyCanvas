#pragma once

#if defined(LAMBDA_PLATFORM_MACOS) && defined(__OBJC__)

#include <Appkit/Appkit.h>

@interface CocoaAppDelegate : NSObject<NSApplicationDelegate>
@end

#else

class CocoaAppDelegate;

#endif
