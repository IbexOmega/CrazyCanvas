#pragma once

#if defined(LAMBDA_PLATFORM_MACOS) && defined(__OBJC__)

#include <AppKit/AppKit.h>

@interface CocoaWindow : NSWindow<NSWindowDelegate>
@end

#else

class CocoaWindow;

#endif
