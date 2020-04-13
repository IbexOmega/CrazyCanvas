#pragma once

#if defined(LAMBDA_PLATFORM_MACOS) && defined(__OBJC__)

#include <Appkit/Appkit.h>

@interface CocoaAppController : NSObject<NSApplicationDelegate>

-(BOOL) applicationShouldTerminateAfterLastWindowClosed:(NSApplication*) sender;

@end

#else

class CocoaAppController;

#endif
