#ifdef LAMBDA_PLATFORM_MACOS
#include "Platform/macOS/MacApplication.h"
#include "Platform/macOS/MacAppController.h"
#include "Platform/macOS/MacInputDevice.h"
#include "Platform/macOS/MacInputCodeTable.h"

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
            if (message.event)
            {
                for (IApplicationMessageHandler* pHandler : m_MessageHandlers)
                {
                    pHandler->HandleEvent(message.event);
                }
                
                [message.event release];
            }
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

    bool MacApplication::Create()
    {
        m_pAppDelegate = [[MacAppController alloc] init];
        [NSApp setDelegate:m_pAppDelegate];
        
        if (!CreateMenu())
        {
            return false;
        }

        if (!m_Window.Init(800, 600))
        {
            return false;
        }
        
        m_Window.Show();
        
        return true;
    }

    bool MacApplication::CreateMenu()
    {
        @autoreleasepool
        {
            NSMenu*     menuBar = [[NSMenu alloc] init];
            NSMenuItem* appMenuItem = [menuBar addItemWithTitle:@"" action:nil keyEquivalent:@""];
            NSMenu*     appMenu = [[NSMenu alloc] init];
            [appMenuItem setSubmenu:appMenu];
            
            [appMenu addItemWithTitle:@"Lambda Engine" action:@selector(orderFrontStandardAboutPanel:) keyEquivalent:@""];
            [appMenu addItem: [NSMenuItem separatorItem]];
            
            //Lambda Engine menu item
            NSMenu* serviceMenu = [[NSMenu alloc] init];
            [[appMenu addItemWithTitle:@"Services" action:nil keyEquivalent:@""] setSubmenu:serviceMenu];
            [appMenu addItem:[NSMenuItem separatorItem]];
            [appMenu addItemWithTitle:@"Hide Lambda Engine" action:@selector(hide:) keyEquivalent:@"h"];
            [[appMenu addItemWithTitle:@"Hide Other" action:@selector(hideOtherApplications:) keyEquivalent:@""] setKeyEquivalentModifierMask:NSEventModifierFlagOption | NSEventModifierFlagCommand];
            [appMenu addItemWithTitle:@"Show All" action:@selector(unhideAllApplications:) keyEquivalent:@""];
            [appMenu addItem:[NSMenuItem separatorItem]];
            [appMenu addItemWithTitle:@"Quit Lambda Engine" action:@selector(terminate:) keyEquivalent:@"q"];
            
            //NSMenuItem* windowMenuItem = []
            
            
            [NSApp setMainMenu:menuBar];
            [NSApp setServicesMenu:serviceMenu];
        }
        
        return true;
    }

    void MacApplication::Destroy()
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
    
        if (!MacInputCodeTable::Init())
        {
            return false;
        }
        
        if (!s_Application.Create())
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
        //Make sure this function is called on the main thread, calling from other threads result in undefined
        ASSERT([NSThread isMainThread]);
        
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
        s_Application.Destroy();
        return true;
    }
}

#endif
