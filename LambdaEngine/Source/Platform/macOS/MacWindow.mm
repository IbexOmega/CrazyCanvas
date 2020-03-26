#ifdef LAMBDA_PLATFORM_MACOS
#include "Platform/macOS/MacWindow.h"
#include "Platform/macOS/MacWindowDelegate.h"

#include <AppKit/AppKit.h>

namespace LambdaEngine
{
    bool MacWindow::Init(uint32 width, uint32 height)
    {
        NSUInteger  windowStyle = NSWindowStyleMaskTitled  | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;
        NSRect      windowRect  = NSMakeRect(0.0f, 0.0f, CGFloat(width), CGFloat(height));
        
        m_pWindow = [[NSWindow alloc] initWithContentRect:windowRect styleMask:windowStyle backing:NSBackingStoreBuffered defer:NO];
        if (!m_pWindow)
        {
            //TODO: Log this
            return false;
        }
        
        m_pDelegate = [[MacWindowDelegate alloc] init];
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
