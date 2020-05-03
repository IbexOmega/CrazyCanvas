#pragma once

#if defined(LAMBDA_PLATFORM_MACOS) && defined(__OBJC__)
#include "Application/API/Console.h"

#include <stdarg.h>

#include <AppKit/AppKit.h>

@interface CocoaConsoleWindow : NSWindow<NSWindowDelegate>
{
    NSTextView*     textView;
    NSScrollView*   scrollView;
    NSDictionary*   consoleColor;
}

- (id) init:(CGFloat) width height:(CGFloat) height;

- (void) appendStringAndScroll:(NSString*) string;
- (void) clearWindow;

- (void) setColor:(LambdaEngine::EConsoleColor) color;

- (NSUInteger) getLineCount;

+ (NSString*) convertStringWithArgs:(const char*) format args:(va_list) args;

@end

#else

class CocoaConsoleWindow;

#endif
