#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#ifdef __OBJC__

#include <AppKit/AppKit.h>

@interface CocoaConsoleWindow : NSWindow<NSWindowDelegate>

-(BOOL) windowShouldClose:(NSWindow* ) sender;

@end

#else

class CocoaConsoleWindow;

#endif
#endif
