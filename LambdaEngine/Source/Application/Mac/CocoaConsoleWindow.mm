#if defined(LAMBDA_PLATFORM_MACOS) && defined(__OBJC__)
#include "Application/Mac/CocoaConsoleWindow.h"
#include "Application/Mac/MacScopedPool.h"

@implementation CocoaConsoleWindow

- (id) init:(CGFloat) width height:(CGFloat) height
{
    SCOPED_AUTORELEASE_POOL();
    
    NSRect      contentRect = NSMakeRect(0.0f, 0.0f, width, height);
    NSUInteger  styleMask   = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;
    
    self = [super initWithContentRect:contentRect styleMask:styleMask backing:NSBackingStoreBuffered defer:NO];
    if (self != nil)
    {
        NSRect contentFrame = [[self contentView] frame];
        scrollView = [[NSScrollView alloc] initWithFrame:contentFrame];
        [scrollView setBorderType:NSNoBorder];
        [scrollView setHasVerticalScroller:YES];
        [scrollView setHasHorizontalScroller:NO];
        [scrollView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
        
        textView = [[NSTextView alloc] initWithFrame:contentFrame];
        [textView setEditable:NO];
        [textView setMinSize:NSMakeSize(0.0f, height)];
        [textView setMaxSize:NSMakeSize(FLT_MAX, FLT_MAX)];
        [textView setVerticallyResizable:YES];
        [textView setHorizontallyResizable:NO];
        [textView setAutoresizingMask:NSViewWidthSizable];
        
        NSTextContainer* container = [textView textContainer];
        [container setContainerSize:NSMakeSize( width, FLT_MAX )];
        [container setWidthTracksTextView:YES];
        
        [self setTitle:@"Lambda Engine Debug Console"];
        [self setContentView:scrollView];
        [self setInitialFirstResponder:textView];
        [self setOpaque:YES];
        
        [self makeKeyAndOrderFront:self];
    }
    
    return self;
}

- (void) dealloc
{
    SCOPED_AUTORELEASE_POOL();
    
    [textView release];
    [scrollView release];
    [consoleColor release];
    
    [super dealloc];
}

- (void) appendStringAndScroll:(NSString *)string
{
    SCOPED_AUTORELEASE_POOL();
    
    NSAttributedString* attributedString = [[NSAttributedString alloc] initWithString:string attributes:consoleColor];
    
    NSTextStorage* storage = [textView textStorage];
    [storage beginEditing];
    [storage appendAttributedString:attributedString];
    [storage setFont:[NSFont fontWithName:@"Courier" size:12.0f]];
    [storage endEditing];
    
    NSString* textString = [textView string];
    [textView scrollRangeToVisible:NSMakeRange([textString length], 0)];
    
    [attributedString release];
}

- (void) clearWindow
{
    
}

- (void) setColor:(LambdaEngine::EConsoleColor) color
{
    using namespace LambdaEngine;
    
    SCOPED_AUTORELEASE_POOL();
    
    if (consoleColor)
    {
        [consoleColor release];
    }
    
    NSMutableArray* colors       = [[NSMutableArray alloc] init];
    NSMutableArray* attributes   = [[NSMutableArray alloc] init];
    [attributes addObject:NSForegroundColorAttributeName];
    [attributes addObject:NSBackgroundColorAttributeName];

    //Add foreground color
    [colors addObject:[NSColor colorWithSRGBRed:0.1f green:0.1f blue:0.1f alpha:0.1f]];

    //Add background color
    if (color == EConsoleColor::COLOR_WHITE)
    {
        [colors addObject:[NSColor colorWithSRGBRed:1.0f green:1.0f blue:1.0f alpha:1.0f]];
    }
    else if (color == EConsoleColor::COLOR_RED)
    {
        [colors addObject:[NSColor colorWithSRGBRed:1.0f green:0.0f blue:0.0f alpha:1.0f]];
    }
    else if (color == EConsoleColor::COLOR_GREEN)
    {
        [colors addObject:[NSColor colorWithSRGBRed:0.0f green:1.0f blue:0.0f alpha:1.0f]];
    }
    else if (color == EConsoleColor::COLOR_YELLOW)
    {
        [colors addObject:[NSColor colorWithSRGBRed:1.0f green:1.0f blue:0.0f alpha:1.0f]];
    }
    
    consoleColor = [[NSDictionary alloc] initWithObjects:colors forKeys:attributes];

    [colors release];
    [attributes release];
}

- (BOOL) windowShouldClose:(NSWindow* ) sender
{
    SCOPED_AUTORELEASE_POOL();
    
    [sender release];
    return YES;
}

+ (NSString*) convertStringWithArgs:(const char *)format args:(va_list)args
{
    NSString* tempFormat    = [NSString stringWithUTF8String:format];
    NSString* result        = [[NSString alloc] initWithFormat:tempFormat arguments:args];
    return result;
}

@end

#endif
