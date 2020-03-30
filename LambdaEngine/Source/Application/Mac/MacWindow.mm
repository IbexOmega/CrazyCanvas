#ifdef LAMBDA_PLATFORM_MACOS
#include "Application/Mac/MacWindow.h"
#include "Application/Mac/CocoaWindow.h"
#include "Application/Mac/CocoaWindowDelegate.h"

namespace LambdaEngine
{
    bool MacWindow::Init(uint32 width, uint32 height)
    {
        NSUInteger  windowStyle = NSWindowStyleMaskTitled  | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;
        NSRect      windowRect  = NSMakeRect(0.0f, 0.0f, CGFloat(width), CGFloat(height));
        
        m_pWindow = [[CocoaWindow alloc] initWithContentRect:windowRect styleMask:windowStyle backing:NSBackingStoreBuffered defer:NO];
        if (!m_pWindow)
        {
            //TODO: Log this
            return false;
        }
        
        m_pDelegate = [[CocoaWindowDelegate alloc] init];
        if (!m_pDelegate)
        {
            //TODO: Log this
            return false;
        }
        
        [m_pWindow setDelegate:m_pDelegate];
        [m_pWindow setTitle:@"Lambda Game Engine"];
        return true;
    }

    void MacWindow::Show()
    {
        [m_pWindow makeKeyAndOrderFront:m_pWindow];
    }

    void MacWindow::Release()
    {
        [m_pWindow release];
        [m_pDelegate release];
    }
}

#endif
