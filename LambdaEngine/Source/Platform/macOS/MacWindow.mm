#ifdef LAMBDA_PLATFORM_MACOS
#include "Platform/macOS/MacWindow.h"

namespace LambdaEngine
{
    MacWindow::MacWindow()
        : m_Window(nullptr),
        m_Delegate(nullptr)
    {
    }

    bool MacWindow::Init(uint32 width, uint32 height)
    {
        NSUInteger  windowStyle = NSWindowStyleMaskTitled  | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;
        NSRect      windowRect  = NSMakeRect(0, 0, width, height);
        
        m_Window = [[NSWindow alloc] initWithContentRect:windowRect styleMask:windowStyle backing:NSBackingStoreBuffered defer:NO];
        if (!m_Window)
        {
            //TODO: Log this
            return false;
        }
        
        m_Delegate = [[MacWindowDelegate alloc] init];
        if (!m_Delegate)
        {
            //TODO: Log this
            return false;
        }
        
        [m_Window setDelegate:m_Delegate];
        [m_Window setTitle:@"Lambda Game Engine"];
        return true;
    }

    void MacWindow::Show()
    {
        [m_Window makeKeyAndOrderFront:m_Window];
    }

    void MacWindow::Release()
    {
        [m_Window release];
        [m_Delegate release];
    }
}

#endif
