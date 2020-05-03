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
    
    MacApplication* pMacApplication = MacApplication::Get();
    if (pMacApplication)
    {
        MacEvent storedEvent = { };
        storedEvent.pNotification = [notification retain];
        storedEvent.pEventWindow  = [self retain];
        
        pMacApplication->StoreEvent(&storedEvent);
    }
}

- (void) windowDidResize:(NSNotification* ) notification
{
    using namespace LambdaEngine;
    
    MacApplication* pMacApplication = MacApplication::Get();
    if (pMacApplication)
    {
        MacEvent storedEvent = { };
        storedEvent.pNotification = [notification retain];
        storedEvent.pEventWindow  = [self retain];
        
        const NSRect contentRect = [[self contentView] frame];
        storedEvent.Size = contentRect.size;
        
        pMacApplication->StoreEvent(&storedEvent);
    }
}

- (void) windowDidMove:(NSNotification* ) notification
{
    using namespace LambdaEngine;
    
    MacApplication* pMacApplication = MacApplication::Get();
    if (pMacApplication)
    {
        MacEvent storedEvent = { };
        storedEvent.pNotification = [notification retain];
        storedEvent.pEventWindow  = [self retain];
        
        const NSRect contentRect = [self contentRectForFrameRect:[self frame]];
        storedEvent.Position = contentRect.origin;
        
        pMacApplication->StoreEvent(&storedEvent);
    }
}

- (void) windowDidMiniaturize:(NSNotification*) notification
{
    using namespace LambdaEngine;
    
    MacApplication* pMacApplication = MacApplication::Get();
    if (pMacApplication)
    {
        MacEvent storedEvent = { };
        storedEvent.pNotification = [notification retain];
        storedEvent.pEventWindow  = [self retain];
        
        const NSRect contentRect = [self contentRectForFrameRect:[self frame]];
        storedEvent.Size = contentRect.size;
        
        pMacApplication->StoreEvent(&storedEvent);
    }
}

- (void) windowDidDeminiaturize:(NSNotification*) notification
{
    using namespace LambdaEngine;
    
    MacApplication* pMacApplication = MacApplication::Get();
    if (pMacApplication)
    {
        MacEvent storedEvent = { };
        storedEvent.pNotification = [notification retain];
        storedEvent.pEventWindow  = [self retain];
        
        const NSRect contentRect = [[self contentView] frame];
        storedEvent.Size = contentRect.size;
        
        pMacApplication->StoreEvent(&storedEvent);
    }
}

- (void) windowDidEnterFullScreen:(NSNotification*) notification
{
    using namespace LambdaEngine;
    
    MacApplication* pMacApplication = MacApplication::Get();
    if (pMacApplication)
    {
        MacEvent storedEvent = { };
        storedEvent.pNotification = [notification retain];
        storedEvent.pEventWindow  = [self retain];
        
        const NSRect contentRect = [[self contentView] frame];
        storedEvent.Size = contentRect.size;
        
        pMacApplication->StoreEvent(&storedEvent);
    }
}

- (void) windowDidExitFullScreen:(NSNotification*) notification
{
    using namespace LambdaEngine;
    
    MacApplication* pMacApplication = MacApplication::Get();
    if (pMacApplication)
    {
        MacEvent storedEvent = { };
        storedEvent.pNotification = [notification retain];
        storedEvent.pEventWindow  = [self retain];
        
        const NSRect contentRect = [[self contentView] frame];
        storedEvent.Size = contentRect.size;
        
        pMacApplication->StoreEvent(&storedEvent);
    }
}

- (void) windowDidBecomeKey:(NSNotification*) notification
{
    using namespace LambdaEngine;
    
    MacApplication* pMacApplication = MacApplication::Get();
    if (pMacApplication)
    {
        MacEvent storedEvent = { };
        storedEvent.pNotification = [notification retain];
        storedEvent.pEventWindow  = [self retain];
        
        pMacApplication->StoreEvent(&storedEvent);
    }
}

- (void) windowDidResignKey:(NSNotification*) notification
{
    using namespace LambdaEngine;
    
    MacApplication* pMacApplication = MacApplication::Get();
    if (pMacApplication)
    {
        MacEvent storedEvent = { };
        storedEvent.pNotification = [notification retain];
        storedEvent.pEventWindow  = [self retain];
        
        pMacApplication->StoreEvent(&storedEvent);
    }
}

@end

#endif
