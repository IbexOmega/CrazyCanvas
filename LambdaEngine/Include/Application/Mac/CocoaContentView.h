#pragma once

#if defined(LAMBDA_PLATFORM_MACOS) && defined(__OBJC__)

#include <Appkit/Appkit.h>
#include <MetalKit/MetalKit.h>

@interface CocoaContentView : MTKView

- (BOOL) acceptsFirstResponder;

- (void) keyDown:(NSEvent*) event;
- (void) keyUp:(NSEvent*) event;

@end

#else

class CocoaContentView;

#endif
