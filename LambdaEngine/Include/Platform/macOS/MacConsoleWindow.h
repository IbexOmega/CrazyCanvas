#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#ifdef __OBJC__

#include <Cocoa/Cocoa.h>
#include <AppKit/AppKit.h>

@interface MacConsoleWindow : NSWindow<NSWindowDelegate>

-(BOOL) windowShouldClose:(NSWindow* ) sender;

@end

#else

class MacConsoleWindow;

#endif
#endif
