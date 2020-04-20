#ifdef LAMBDA_PLATFORM_MACOS
#include "Log/Log.h"

#include "Memory/Memory.h"

#include "Application/Mac/MacConsole.h"
#include "Application/Mac/MacApplication.h"
#include "Application/Mac/CocoaAppController.h"
#include "Application/Mac/MacScopedPool.h"

#include "Input/Mac/MacInputDevice.h"
#include "Input/Mac/MacInputCodeTable.h"

#include "Threading/Mac/MacMainThread.h"

#include <Appkit/Appkit.h>

namespace LambdaEngine
{
    MacApplication* MacApplication::s_pApplication = nullptr;

    MacApplication::~MacApplication()
    {
        SCOPED_AUTORELEASE_POOL();
        
        [m_pAppDelegate release];
        SAFEDELETE(m_pWindow);
    }

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
        return m_pWindow;
    }

    const Window* MacApplication::GetWindow() const
    {
        return m_pWindow;
    }

    bool MacApplication::Init()
    {
        SCOPED_AUTORELEASE_POOL();
        
        m_pAppDelegate = [[CocoaAppController alloc] init];
        [NSApp setDelegate:m_pAppDelegate];
        
        if (!InitMenu())
        {
            LOG_ERROR("[MacApplication]: Failed to initialize the application menu");
            return false;
        }
        
        m_pWindow = (MacWindow*)MacApplication::CreateWindow("Lambda Game Engine", 1440, 900);
        if (!m_pWindow)
        {
            return false;
        }
        
        m_pWindow->Show();
        
        return true;
    }

    bool MacApplication::InitMenu()
    {
        SCOPED_AUTORELEASE_POOL();
        
        MacMainThread::MakeCall(^
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
            
            //Window menu
            NSMenuItem* windowMenuItem  = [menuBar addItemWithTitle:@"" action:nil keyEquivalent:@""];
            NSMenu*     windowMenu      = [[NSMenu alloc] initWithTitle:@"Window"];
            [windowMenuItem setSubmenu:windowMenu];
            
            [windowMenu addItemWithTitle:@"Minimize" action:@selector(performMiniaturize:) keyEquivalent:@"m"];
            [windowMenu addItemWithTitle:@"Zoom" action:@selector(performZoom:) keyEquivalent:@""];
            [windowMenu addItem:[NSMenuItem separatorItem]];
            
            [windowMenu addItemWithTitle:@"Bring All to Front" action:@selector(arrangeInFront:) keyEquivalent:@""];
            [windowMenu addItem:[NSMenuItem separatorItem]];
            
            [[windowMenu addItemWithTitle:@"Enter Full Screen" action:@selector(toggleFullScreen:) keyEquivalent:@"f"] setKeyEquivalentModifierMask:NSEventModifierFlagControl | NSEventModifierFlagCommand];
            
            SEL setAppleMenuSelector = NSSelectorFromString(@"setAppleMenu:");
            [NSApp performSelector:setAppleMenuSelector withObject:appMenu];
            
            [NSApp setMainMenu:menuBar];
            [NSApp setWindowsMenu:windowMenu];
            [NSApp setServicesMenu:serviceMenu];
        }, true);
    
        return true;
    }

    void MacApplication::Terminate()
    {
        s_pApplication->m_IsTerminating = true;
    }

    Window* MacApplication::CreateWindow(const char* pTitle, uint32 width, uint32 height)
    {
        MacWindow* pWindow = DBG_NEW MacWindow();
        if (!pWindow->Init(pTitle, width, height))
        {
            SAFEDELETE(pWindow);
        }
        
        return pWindow;
    }

    IInputDevice* MacApplication::CreateInputDevice(EInputMode)
    {
        MacInputDevice* pInputDevice = DBG_NEW MacInputDevice();
        s_pApplication->AddMessageHandler(pInputDevice);

        return pInputDevice;
    }

    bool MacApplication::PreInit()
    {
        SCOPED_AUTORELEASE_POOL();
        
        [NSApplication sharedApplication];
        
        ASSERT(NSApp != nil);
        
        [NSApp activateIgnoringOtherApps:YES];
        [NSApp setPresentationOptions:NSApplicationPresentationDefault];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

        MacMainThread::PreInit();
        
        s_pApplication = DBG_NEW MacApplication();
        if (!s_pApplication->Init())
        {
            return false;
        }
        
        if (!MacInputCodeTable::Init())
        {
            return false;
        }
        
        //Process events in the queue
        ProcessMessages();
        
        [NSApp finishLaunching];
        return true;
    }

    bool MacApplication::Tick()
    {
        MacMainThread::Tick();
        
        bool shouldExit = ProcessMessages();
        s_pApplication->ProcessBufferedMessages();

        return shouldExit;
    }

    bool MacApplication::ProcessMessages()
    {
        SCOPED_AUTORELEASE_POOL();
        
        //Make sure this function is called on the main thread, calling from other threads result in undefined
        ASSERT([NSThread isMainThread]);
        
        if (s_pApplication)
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
                s_pApplication->BufferEvent(event);

                [NSApp sendEvent:event];
                [NSApp updateWindows];
                
                if (s_pApplication->m_IsTerminating)
                {
                    return false;
                }
            }
        }
        
        return true;
    }

    void MacApplication::BufferEvent(NSEvent* event)
    {
        SCOPED_AUTORELEASE_POOL();
        
        MacMessage message = { };
        message.event = [event retain];

        m_BufferedMessages.push_back(message);
    }

    bool MacApplication::PostRelease()
    {
        SAFEDELETE(s_pApplication);
        
        MacMainThread::PostRelease();
        return true;
    }
}

#endif
