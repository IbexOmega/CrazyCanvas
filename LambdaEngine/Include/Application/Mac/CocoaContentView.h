#pragma once

#if defined(LAMBDA_PLATFORM_MACOS) && defined(__OBJC__)

#include <Appkit/Appkit.h>
#include <MetalKit/MetalKit.h>

@interface CocoaContentView : MTKView<NSTextInputClient>
@end

#else

class CocoaContentView;

#endif
