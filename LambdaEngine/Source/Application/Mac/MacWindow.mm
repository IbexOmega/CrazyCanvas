#ifdef LAMBDA_PLATFORM_MACOS
#include "Log/Log.h"

#include "Application/Mac/MacWindow.h"
#include "Application/Mac/MacApplication.h"
#include "Application/Mac/CocoaWindow.h"
#include "Application/Mac/CocoaContentView.h"
#include "Application/Mac/MacScopedPool.h"

#include "Threading/Mac/MacMainThread.h"

namespace LambdaEngine
{
    MacWindow::MacWindow()
        : IWindow()
    {
    }

    MacWindow::~MacWindow()
    {
        SCOPED_AUTORELEASE_POOL();
        
        [m_pWindow release];
        [m_pView release];
    }

    bool MacWindow::Init(const char* pTitle, uint32 width, uint32 height)
    {
        SCOPED_AUTORELEASE_POOL();
        
        NSUInteger  windowStyle = NSWindowStyleMaskTitled  | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;
        NSRect      windowRect  = NSMakeRect(0.0f, 0.0f, CGFloat(width), CGFloat(height));
        
        m_pWindow = [[CocoaWindow alloc] initWithContentRect:windowRect styleMask:windowStyle backing:NSBackingStoreBuffered defer:NO];
        if (!m_pWindow)
        {
            LOG_ERROR("[MacWindow]: Failed to create NSWindow");
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
        [m_pWindow setAcceptsMouseMovedEvents:YES];
        [m_pWindow setContentView:m_pView];
        [m_pWindow setRestorable:NO];
        [m_pWindow makeFirstResponder:m_pView];
        
        const NSWindowCollectionBehavior behavior = NSWindowCollectionBehaviorFullScreenPrimary | NSWindowCollectionBehaviorManaged;
        [m_pWindow setCollectionBehavior:behavior];
        
        return true;
    }

    void MacWindow::Show()
    {
        MacMainThread::MakeCall(^
        {
            [m_pWindow makeKeyAndOrderFront:m_pWindow];
        }, true);
    }

    void MacWindow::Close()
    {
        MacMainThread::MakeCall(^
        {
            [m_pWindow performClose:m_pWindow];
        }, true);
    }

    void MacWindow::Minimize()
    {
        MacMainThread::MakeCall(^
        {
            [m_pWindow miniaturize:m_pWindow];
        }, true);
    }

    void MacWindow::Maximize()
    {
        MacMainThread::MakeCall(^
        {
            if ([m_pWindow isMiniaturized])
            {
                [m_pWindow deminiaturize:m_pWindow];
            }
            
            [m_pWindow zoom:m_pWindow];
        }, true);
    }

    void MacWindow::Restore()
    {
        MacMainThread::MakeCall(^
        {
            if ([m_pWindow isMiniaturized])
            {
                [m_pWindow deminiaturize:m_pWindow];
            }
            
            if ([m_pWindow isZoomed])
            {
                [m_pWindow zoom:m_pWindow];
            }
        }, true);
    }

    void MacWindow::ToggleFullscreen()
    {
        MacMainThread::MakeCall(^
        {
            [m_pWindow toggleFullScreen:m_pWindow];
        }, true);
    }

    void MacWindow::SetTitle(const char* pTitle)
    {
        SCOPED_AUTORELEASE_POOL();
        
        NSString* title = [NSString stringWithUTF8String:pTitle];
        
        MacMainThread::MakeCall(^
        {
            [m_pWindow setTitle:title];
        }, true);
    }

    uint16 MacWindow::GetWidth() const
    {
        SCOPED_AUTORELEASE_POOL();
        
        __block NSRect contentRect;
        MacMainThread::MakeCall(^
        {
            contentRect = [m_pWindow contentRectForFrameRect:[m_pWindow frame]];
        }, true);
        
        return uint16(contentRect.size.width);
    }

    uint16 MacWindow::GetHeight() const
    {
        SCOPED_AUTORELEASE_POOL();
        
        __block NSRect contentRect;
        MacMainThread::MakeCall(^
        {
            contentRect = [m_pWindow contentRectForFrameRect:[m_pWindow frame]];
        }, true);
        
        return uint16(contentRect.size.height);
    }

    void* MacWindow::GetHandle() const
    {
        return (void*)m_pWindow;
    }

    const void* MacWindow::GetView() const
    {
        return (const void*)m_pView;
    }
}

#endif
