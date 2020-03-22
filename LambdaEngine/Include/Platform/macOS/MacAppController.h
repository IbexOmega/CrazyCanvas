#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#ifdef __OBJC__

#include <Appkit/Appkit.h>
#include <Foundation/Foundation.h>

@interface MacAppController : NSObject<NSApplicationDelegate>

-(BOOL) applicationShouldTerminateAfterLastWindowClosed:(NSApplication*) sender;

@end

#else

class MacAppController;

#endif
#endif
