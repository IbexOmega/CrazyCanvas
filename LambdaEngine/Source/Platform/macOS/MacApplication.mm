#ifdef LAMBDA_PLATFORM_MACOS
#include "Platform/macOS/MacApplication.h"
#include "Platform/macOS/MacAppController.h"
#include "Platform/macOS/MacWindowDelegate.h"

namespace LambdaEngine
{
    MacApplication MacApplication::s_Application = MacApplication();

    void MacApplication::AddMessageHandler(IApplicationMessageHandler* pHandler)
    {
    }

    void MacApplication::RemoveMessageHandler(IApplicationMessageHandler* pHandler)
    {
    }

    void MacApplication::ProcessBufferedMessages()
    {
    }

    Window* MacApplication::GetWindow()
    {
        return &m_Window;
    }

    const Window* MacApplication::GetWindow() const
    {
        return &m_Window;
    }

    bool MacApplication::Init()
    {
        m_pAppDelegate = [[MacAppController alloc] init];
        [NSApp setDelegate:m_pAppDelegate];
        
        //TODO: Other setup here

        if (!m_Window.Init(800, 600))
        {
            return false;
        }
        
        m_Window.Show();
        
        return true;
    }

    void MacApplication::Release()
    {
        [m_pAppDelegate release];
        m_Window.Release();
    }

    void MacApplication::Terminate()
    {
        s_Application.m_IsTerminating = true;
    }

    InputDevice* MacApplication::CreateInputDevice()
    {
        return nullptr;
    }

    bool MacApplication::PreInit()
    {
        [NSApplication sharedApplication];
        if (NSApp != nil)
        {
            [NSApp activateIgnoringOtherApps:YES];
            [NSApp setPresentationOptions:NSApplicationPresentationDefault];
            [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        }
        else
        {
            //TODO: Log error
            return false;
        }
    
        if (!s_Application.Init())
        {
            return false;
        }
        
        [NSApp finishLaunching];
        
        return true;
    }

    bool MacApplication::Tick()
    {
        return ProcessMessages();
    }

    bool MacApplication::ProcessMessages()
    {
        @autoreleasepool
        {
            NSEvent* event = nil;
            while (true)
            {
                event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES];
                if (event == nil)
                {
                    break;
                }
                
                [NSApp sendEvent:event];
                [NSApp updateWindows];
                
                if (s_Application.m_IsTerminating)
                {
                    return false;
                }
            }
        }
        
        return true;
    }

    bool MacApplication::PostRelease()
    {
        s_Application.Release();
        return true;
    }
}

#endif
