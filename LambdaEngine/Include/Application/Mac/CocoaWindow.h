#pragma once

#if defined(LAMBDA_PLATFORM_MACOS) && defined(__OBJC__)

#include <AppKit/AppKit.h>

@interface CocoaWindow : NSWindow

- (BOOL)canBecomeKeyWindow;
- (BOOL)canBecomeMainWindow;
- (BOOL)acceptsMouseMovedEvents;
- (BOOL)acceptsFirstResponder;

@end

#else

class CocoaWindow;

#endif
