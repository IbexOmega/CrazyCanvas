#if defined(LAMBDA_PLATFORM_MACOS) && defined(__OBJC__)
#include "Application/Mac/CocoaContentView.h"
#include "Application/Mac/MacApplication.h"
#include "Application/Mac/MacScopedPool.h"

@implementation CocoaContentView

- (BOOL) canBecomeKeyView
{
    return YES;
}

- (BOOL) acceptsFirstResponder
{
    return YES;
}

- (BOOL) hasMarkedText
{
    return NO;
}

- (BOOL) wantsUpdateLayer
{
    return YES;
}

- (BOOL) acceptsFirstMouse:(NSEvent* ) event
{
    return YES;
}

- (NSArray*) validAttributesForMarkedText
{
    return [NSArray array];
}

- (void) viewWillMoveToWindow:(NSWindow*) window
{
    NSTrackingArea* trackingArea = [[NSTrackingArea alloc] initWithRect:[self bounds] options: (NSTrackingMouseEnteredAndExited | NSTrackingActiveAlways) owner:self userInfo:nil];
    [self addTrackingArea:trackingArea];
}

- (void) keyDown:(NSEvent*) event
{
    // Interpret key event and make sure we get a KeyTyped event
    [self interpretKeyEvents:[NSArray arrayWithObject: event]];
	LambdaEngine::MacApplication::Get()->StoreNSEvent(event);
}

- (void) keyUp:(NSEvent*) event
{
	LambdaEngine::MacApplication::Get()->StoreNSEvent(event);
}

- (void) mouseDown:(NSEvent*) event
{
	LambdaEngine::MacApplication::Get()->StoreNSEvent(event);
}

- (void) mouseDragged:(NSEvent*) event
{
	LambdaEngine::MacApplication::Get()->StoreNSEvent(event);
}

- (void) mouseUp:(NSEvent*) event
{
	LambdaEngine::MacApplication::Get()->StoreNSEvent(event);
}

- (void) mouseMoved:(NSEvent*) event
{
	LambdaEngine::MacApplication::Get()->StoreNSEvent(event);
}

- (void) rightMouseDown:(NSEvent*) event
{
	LambdaEngine::MacApplication::Get()->StoreNSEvent(event);
}

- (void) rightMouseDragged:(NSEvent*) event
{
	LambdaEngine::MacApplication::Get()->StoreNSEvent(event);
}

- (void) rightMouseUp:(NSEvent*) event
{
	LambdaEngine::MacApplication::Get()->StoreNSEvent(event);
}

- (void) otherMouseDown:(NSEvent*) event
{
	LambdaEngine::MacApplication::Get()->StoreNSEvent(event);
}

- (void) otherMouseDragged:(NSEvent*) event
{
	LambdaEngine::MacApplication::Get()->StoreNSEvent(event);
}

- (void) otherMouseUp:(NSEvent*) event
{
	LambdaEngine::MacApplication::Get()->StoreNSEvent(event);
}

- (void) scrollWheel:(NSEvent*) event
{
	LambdaEngine::MacApplication::Get()->StoreNSEvent(event);
}

- (void) insertText:(id) string replacementRange:(NSRange) replacementRange
{
    SCOPED_AUTORELEASE_POOL();
    
    // Get characters
    NSString* characters = nullptr;
    if ([string isKindOfClass:[NSAttributedString class]])
    {
        NSAttributedString* attributedString = (NSAttributedString*)string;
        characters = [attributedString string];
    }
    else
    {
        characters = (NSString*)string;
    }
    
    // Store this as a special type of event
    using namespace LambdaEngine;
    
	MacEvent event = { };
	event.pKeyTypedText = [characters copy];
	
	MacApplication::Get()->StoreEvent(event);
}

/*
 * Implemented methods to avoid crashing
 */
- (void) doCommandBySelector:(SEL)selector
{
}

- (nullable NSAttributedString*) attributedSubstringForProposedRange:(NSRange)range actualRange:(nullable NSRangePointer)actualRange
{
    return nil;
}

- (NSUInteger) characterIndexForPoint:(NSPoint)point
{
    return 0;
}

- (NSRect) firstRectForCharacterRange:(NSRange)range actualRange:(nullable NSRangePointer)actualRange
{
    const NSRect frame = [self frame];
    return NSMakeRect(frame.origin.x, frame.origin.y, 0.0, 0.0);
}

- (NSRange)markedRange
{
    return NSMakeRange(NSNotFound, 0);
}

- (NSRange) selectedRange
{
    return NSMakeRange(NSNotFound, 0);
}

- (void)setMarkedText:(nonnull id)string selectedRange:(NSRange)selectedRange replacementRange:(NSRange)replacementRange
{
}

- (void)unmarkText
{
}

- (void) viewDidChangeBackingProperties
{
    CGFloat backingScaleFactor = [[self window] backingScaleFactor];
    [[self layer] setContentsScale:backingScaleFactor];
}

- (void) mouseExited:(NSEvent* ) event
{
	LambdaEngine::MacApplication::Get()->StoreNSEvent(event);
}

- (void) mouseEntered:(NSEvent* )event
{
    LambdaEngine::MacApplication::Get()->StoreNSEvent(event);
}

@end

#endif
