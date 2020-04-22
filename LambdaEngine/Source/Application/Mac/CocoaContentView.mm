#if defined(LAMBDA_PLATFORM_MACOS) && defined(__OBJC__)
#include "Application/Mac/CocoaContentView.h"

@implementation CocoaContentView

- (BOOL) acceptsFirstResponder
{
    return YES;
}

- (void) keyDown:(NSEvent*) event
{
}

- (void) keyUp:(NSEvent*) event
{
}

@end

#endif
