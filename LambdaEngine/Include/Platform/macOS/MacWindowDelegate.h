#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#include <Appkit/Appkit.h>
#include <Foundation/Foundation.h>

@interface MacWindowDelegate : NSObject<NSWindowDelegate>

- (void) windowWillClose:(NSNotification *)notification;

@end

#endif


