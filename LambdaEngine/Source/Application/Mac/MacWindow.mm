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
        : Window()
    {
    }

    MacWindow::~MacWindow()
    {
        SCOPED_AUTORELEASE_POOL();
        
        [m_pWindow release];
        [m_pView release];
    }

    bool MacWindow::Init(const WindowDesc* pDesc)
    {
		VALIDATE(pDesc != nullptr);
		
        SCOPED_AUTORELEASE_POOL();
        
		NSUInteger windowStyle = 0;
		if (pDesc->Style != 0)
		{
			windowStyle = NSWindowStyleMaskTitled;
			if (pDesc->Style & WINDOW_STYLE_FLAG_CLOSABLE)
			{
				windowStyle |= NSWindowStyleMaskClosable;
			}
			if (pDesc->Style & WINDOW_STYLE_FLAG_RESIZEABLE)
			{
				windowStyle |= NSWindowStyleMaskResizable;
			}
			if (pDesc->Style & WINDOW_STYLE_FLAG_MINIMIZABLE)
			{
				windowStyle |= NSWindowStyleMaskMiniaturizable;
			}
		}
		else
		{
			windowStyle = NSWindowStyleMaskBorderless;
		}
		
		const NSRect windowRect = NSMakeRect(0.0f, 0.0f, CGFloat(pDesc->Width), CGFloat(pDesc->Height));
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
        
		if (pDesc->Style & WINDOW_STYLE_FLAG_TITLED)
		{
			if (pDesc->pTitle)
			{
				NSString* title = [NSString stringWithUTF8String:pDesc->pTitle];
				[m_pWindow setTitle:title];
			}
		}
		
        [m_pWindow setAcceptsMouseMovedEvents:YES];
        [m_pWindow setContentView:m_pView];
        [m_pWindow setRestorable:NO];
        [m_pWindow makeFirstResponder:m_pView];
        
		// Disable fullscreen toggle if window is not resizeable
        NSWindowCollectionBehavior behavior = NSWindowCollectionBehaviorManaged;
		if (pDesc->Style & WINDOW_STYLE_FLAG_RESIZEABLE)
		{
			behavior |= NSWindowCollectionBehaviorFullScreenPrimary;
		}
		else
		{
			behavior |= NSWindowCollectionBehaviorFullScreenAuxiliary;
		}
		
        [m_pWindow setCollectionBehavior:behavior];
        
		// Set styleflags
		m_StyleFlags = pDesc->Style;
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
		if (m_StyleFlags & WINDOW_STYLE_FLAG_CLOSABLE)
		{
			MacMainThread::MakeCall(^
			{
				[m_pWindow performClose:m_pWindow];
			}, true);
		}
    }

    void MacWindow::Minimize()
    {
		if (m_StyleFlags & WINDOW_STYLE_FLAG_MINIMIZABLE)
		{
			MacMainThread::MakeCall(^
			{
				[m_pWindow miniaturize:m_pWindow];
			}, true);
		}
    }

    void MacWindow::Maximize()
    {
		if (m_StyleFlags & WINDOW_STYLE_FLAG_MAXIMIZABLE)
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
    }

	bool MacWindow::IsActiveWindow() const
	{
        NSWindow* keyWindow = [NSApp keyWindow];
		return keyWindow == m_pWindow;
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
		if (m_StyleFlags & WINDOW_STYLE_FLAG_RESIZEABLE)
		{
			MacMainThread::MakeCall(^
			{
				[m_pWindow toggleFullScreen:m_pWindow];
			}, true);
		}
    }

    void MacWindow::SetTitle(const char* pTitle)
    {
        SCOPED_AUTORELEASE_POOL();
        
        NSString* title = [NSString stringWithUTF8String:pTitle];
		if (m_StyleFlags & WINDOW_STYLE_FLAG_TITLED)
		{
			MacMainThread::MakeCall(^
			{
				[m_pWindow setTitle:title];
			}, true);
		}
    }

	void MacWindow::SetPosition(int32 x, int32 y)
	{
		MacMainThread::MakeCall(^
		{
			NSRect frame = [m_pWindow frame];
			
			// TODO: Make sure this is correct
			[m_pWindow setFrameOrigin:NSMakePoint(x, y - frame.size.height + 1)];
		}, true);
	}

	void MacWindow::GetPosition(int32* pPosX, int32* pPosY) const
	{
		__block NSRect frame;
		MacMainThread::MakeCall(^
		{
			frame = [m_pWindow frame];
		}, true);
		
		(*pPosX) = frame.origin.x;
		(*pPosY) = frame.origin.y;
	}

	void MacWindow::SetSize(uint16 width, uint16 height)
	{
        MacMainThread::MakeCall(^
        {
			NSRect frame = [m_pWindow frame];
			frame.size.width 	= width;
			frame.size.height 	= height;
			[m_pWindow setFrame: frame display: YES animate: YES];
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

	float32 MacWindow::GetClientAreaScale() const
	{
		return 1.0f;
	}
}

#endif
