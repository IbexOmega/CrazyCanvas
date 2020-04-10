#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#ifdef __OBJC__

#include <Appkit/Appkit.h>

@interface CocoaAppController : NSObject<NSApplicationDelegate>

-(BOOL) applicationShouldTerminateAfterLastWindowClosed:(NSApplication*) sender;

@end

#else

class CocoaAppController;

#endif
#endif
