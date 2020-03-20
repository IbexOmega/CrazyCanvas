#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#include <Appkit/Appkit.h>
#include <Foundation/Foundation.h>

@interface MacAppController : NSObject<NSApplicationDelegate>

-(BOOL) applicationShouldTerminateAfterLastWindowClosed:(NSApplication*) sender;

@end

#endif
