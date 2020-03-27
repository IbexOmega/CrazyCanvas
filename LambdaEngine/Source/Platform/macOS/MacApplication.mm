#ifdef LAMBDA_PLATFORM_MACOS
#include "Platform/macOS/MacApplication.h"
#include "Platform/macOS/MacAppController.h"
#include "Platform/macOS/MacWindowDelegate.h"
#include "Platform/macOS/MacInputDevice.h"

#include <Appkit/Appkit.h>

namespace LambdaEngine
{
    MacApplication MacApplication::s_Application = MacApplication();

    void MacApplication::AddMessageHandler(IApplicationMessageHandler* pHandler)
    {
        //Check first so that this handler is not already added
		const uint32 count = uint32(m_MessageHandlers.size());
		for (uint32 i = 0; i < count; i++)
		{
			if (pHandler == m_MessageHandlers[i])
			{
				return;
			}
		}

		//Add new handler
		m_MessageHandlers.emplace_back(pHandler);
    }

    void MacApplication::RemoveMessageHandler(IApplicationMessageHandler* pHandler)
    {
        const uint32 count = uint32(m_MessageHandlers.size());
		for (uint32 i = 0; i < count; i++)
		{
			if (pHandler == m_MessageHandlers[i])
			{
                //A handler must be unique since we check for it when adding a handler, therefore we can break here
				m_MessageHandlers.erase(m_MessageHandlers.begin() + i);
				break;
			}
		}
    }

    void MacApplication::ProcessBufferedMessages()
    {
        for (MacMessage& message : m_BufferedMessages)
        {
            for (IApplicationMessageHandler* pHandler : m_MessageHandlers)
            {
                pHandler->HandleEvent(message.event);
            }
            
            [message.event release];
        }
        
        m_BufferedMessages.clear();
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
        MacInputDevice* pInputDevice = new MacInputDevice();
        s_Application.AddMessageHandler(pInputDevice);

        return pInputDevice;
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
        bool shouldExit = ProcessMessages();
        s_Application.ProcessBufferedMessages();

        return shouldExit;
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
                
                //Buffer event before sending it to the rest of the system
                s_Application.BufferEvent(event);

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

    void MacApplication::BufferEvent(NSEvent* event)
    {
        MacMessage message = { };
        message.event = [event retain];

        m_BufferedMessages.push_back(message);
    }

    bool MacApplication::PostRelease()
    {
        s_Application.Release();
        return true;
    }
}

#endif
