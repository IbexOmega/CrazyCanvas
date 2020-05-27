#if defined(LAMBDA_PLATFORM_MACOS) && defined(__OBJC__)
#include "Log/Log.h"

#include "Application/Mac/CocoaWindow.h"
#include "Application/Mac/MacApplication.h"
#include "Application/Mac/MacScopedPool.h"

#include "Input/Mac/MacInputCodeTable.h"

@implementation CocoaWindow

- (id) initWithContentRect:(NSRect)contentRect styleMask:(NSWindowStyleMask)style backing:(NSBackingStoreType)backingStoreType defer:(BOOL)flag
{
    self = [super initWithContentRect:contentRect styleMask:style backing:backingStoreType defer:flag];
    if (self != nil)
    {
        [self setDelegate:self];
        [self setOpaque:YES];
    }
    
    return self;
}

- (BOOL) canBecomeKeyWindow
{
    return YES;
}

- (BOOL) canBecomeMainWindow
{
    return YES;
}

- (BOOL) acceptsMouseMovedEvents
{
    return YES;
}

- (BOOL) acceptsFirstResponder
{
    return YES;
}

- (void) windowWillClose:(NSNotification* ) notification
{
    using namespace LambdaEngine;
    
	MacEvent event = { };
	event.pNotification = [notification retain];
	event.pEventWindow  = [self retain];
	
	MacApplication::Get()->StoreEvent(event);
}

- (void) windowDidResize:(NSNotification* ) notification
{
    using namespace LambdaEngine;
    
	MacEvent event = { };
	event.pNotification = [notification retain];
	event.pEventWindow  = [self retain];
	
	const NSRect contentRect = [[self contentView] frame];
	event.Size = contentRect.size;
	
	MacApplication::Get()->StoreEvent(event);
}

- (void) windowDidMove:(NSNotification* ) notification
{
    using namespace LambdaEngine;
    
	MacEvent event = { };
	event.pNotification = [notification retain];
	event.pEventWindow  = [self retain];
	
	const NSRect contentRect = [self contentRectForFrameRect:[self frame]];
	event.Position = contentRect.origin;
	
	MacApplication::Get()->StoreEvent(event);
}

- (void) windowDidMiniaturize:(NSNotification*) notification
{
    using namespace LambdaEngine;
    
	MacEvent event = { };
	event.pNotification = [notification retain];
	event.pEventWindow  = [self retain];
	
	const NSRect contentRect = [self contentRectForFrameRect:[self frame]];
	event.Size = contentRect.size;
	
	MacApplication::Get()->StoreEvent(event);
}

- (void) windowDidDeminiaturize:(NSNotification*) notification
{
    using namespace LambdaEngine;

	MacEvent event = { };
	event.pNotification = [notification retain];
	event.pEventWindow  = [self retain];
	
	const NSRect contentRect = [[self contentView] frame];
	event.Size = contentRect.size;
	
	MacApplication::Get()->StoreEvent(event);
}

- (void) windowDidEnterFullScreen:(NSNotification*) notification
{
    using namespace LambdaEngine;
    
	MacEvent event = { };
	event.pNotification = [notification retain];
	event.pEventWindow  = [self retain];
	
	const NSRect contentRect = [[self contentView] frame];
	event.Size = contentRect.size;
	
	MacApplication::Get()->StoreEvent(event);
}

- (void) windowDidExitFullScreen:(NSNotification*) notification
{
    using namespace LambdaEngine;
    
	MacEvent event = { };
	event.pNotification = [notification retain];
	event.pEventWindow  = [self retain];
	
	const NSRect contentRect = [[self contentView] frame];
	event.Size = contentRect.size;
	
	MacApplication::Get()->StoreEvent(event);
}

- (void) windowDidBecomeKey:(NSNotification*) notification
{
    using namespace LambdaEngine;

	MacEvent event = { };
	event.pNotification = [notification retain];
	event.pEventWindow  = [self retain];
	
	MacApplication::Get()->StoreEvent(event);
}

- (void) windowDidResignKey:(NSNotification*) notification
{
    using namespace LambdaEngine;
    
	MacEvent event = { };
	event.pNotification = [notification retain];
	event.pEventWindow  = [self retain];
	
	MacApplication::Get()->StoreEvent(event);
}

@end

#endif
