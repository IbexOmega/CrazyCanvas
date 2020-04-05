#ifdef LAMBDA_PLATFORM_MACOS
#include "Log/Log.h"

#include "Application/Mac/MacWindow.h"
#include "Application/Mac/MacApplication.h"
#include "Application/Mac/CocoaWindow.h"
#include "Application/Mac/CocoaWindowDelegate.h"
#include "Application/Mac/CocoaContentView.h"

namespace LambdaEngine
{
    MacWindow::~MacWindow()
    {
        [m_pWindow release];
        [m_pView release];
        [m_pDelegate release];
    }

    bool MacWindow::Init(const char* pTitle, uint32 width, uint32 height)
    {
        NSUInteger  windowStyle = NSWindowStyleMaskTitled  | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;
        NSRect      windowRect  = NSMakeRect(0.0f, 0.0f, CGFloat(width), CGFloat(height));
        
        m_pWindow = [[CocoaWindow alloc] initWithContentRect:windowRect styleMask:windowStyle backing:NSBackingStoreBuffered defer:NO];
        if (!m_pWindow)
        {
            LOG_ERROR("[MacWindow]: Failed to create NSWindow");
            return false;
        }
        
        m_pDelegate = [[CocoaWindowDelegate alloc] init];
        if (!m_pDelegate)
        {
            LOG_ERROR("[MacWindow]: Failed to create CocoaWindowDelegate");
            return false;
        }
        
        m_pView = [[CocoaContentView alloc] init];
        if (!m_pView)
        {
            LOG_ERROR("[MacWindow]: Failed to create CocoaContentView");
            return false;
        }
        
        NSString* title = [NSString stringWithUTF8String:pTitle];
        [m_pWindow setTitle:title];
        [m_pWindow setDelegate:m_pDelegate];
        [m_pWindow setContentView:m_pView];
        
        return true;
    }

    void MacWindow::Show()
    {
        [m_pWindow makeKeyAndOrderFront:m_pWindow];
    }

    void MacWindow::SetTitle(const char* pTitle)
    {
        NSString* title = [NSString stringWithUTF8String:pTitle];
        [m_pWindow setTitle:title];
        
        MacApplication::ProcessMessages();
    }
}

#endif
