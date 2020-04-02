#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#ifdef __OBJC__

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
#endif
