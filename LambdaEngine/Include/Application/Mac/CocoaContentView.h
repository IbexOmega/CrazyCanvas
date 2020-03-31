#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#ifdef __OBJC__

#include <Appkit/Appkit.h>
#include <MetalKit/MetalKit.h>

@interface CocoaContentView : MTKView
@end

#else

class CocoaContentView;

#endif
#endif
