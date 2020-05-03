#ifdef LAMBDA_PLATFORM_MACOS
#include "Log/Log.h"

#include "Memory/Memory.h"

#include "Application/API/IWindowHandler.h"

#include "Application/Mac/MacConsole.h"
#include "Application/Mac/MacApplication.h"
#include "Application/Mac/MacScopedPool.h"
#include "Application/Mac/MacWindow.h"
#include "Application/Mac/CocoaWindow.h"
#include "Application/Mac/CocoaAppDelegate.h"

#include "Input/Mac/MacInputDevice.h"
#include "Input/Mac/MacInputCodeTable.h"

#include "Threading/Mac/MacMainThread.h"

#include <mutex>

#include <Appkit/Appkit.h>

namespace LambdaEngine
{
    /*
     * MacEvent
     */

    MacEvent::MacEvent()
    {
    }

    MacEvent::MacEvent(const MacEvent& other)
        : pEventWindow((other.pEventWindow) ? [other.pEventWindow retain] : nullptr),
        pEvent((other.pEvent) ? [other.pEvent retain] : nullptr),
        pNotification((other.pNotification) ? [other.pNotification retain] : nullptr),
        pKeyTypedText((other.pKeyTypedText) ? [other.pKeyTypedText retain] : nullptr),
        Size(other.Size),
        Position(other.Position)
    {
    }

    MacEvent::~MacEvent()
    {
        if (pEventWindow)
        {
            [pEventWindow release];
            pEventWindow = nullptr;
        }
        
        if (pEvent)
        {
            [pEvent release];
            pEvent = nullptr;
        }
        
        if (pNotification)
        {
            [pNotification release];
            pNotification = nullptr;
        }
        
        if (pKeyTypedText)
        {
            [pKeyTypedText release];
            pKeyTypedText = nullptr;
        }
    }

    /*
     * Instance
     */
    MacApplication* MacApplication::s_pApplication = nullptr;

    MacApplication::MacApplication()
        : Application()
    {
    	VALIDATE_MSG(s_pApplication == nullptr, "[MacApplication]: An instance of application already exists");
		s_pApplication = this;
    }

    MacApplication::~MacApplication()
    {
        SCOPED_AUTORELEASE_POOL();
        
        VALIDATE_MSG(s_pApplication != nullptr, "[MacApplication]: Instance of application has already been deleted");
		s_pApplication = nullptr;

        for (MacWindow* pWindow : m_Windows)
        {
            SAFERELEASE(pWindow);
        }

        [m_pAppDelegate release];
    }

    bool MacApplication::Init()
    {
        SCOPED_AUTORELEASE_POOL();
        
        m_pAppDelegate = [[CocoaAppDelegate alloc] init];
        [NSApp setDelegate:m_pAppDelegate];
        
        if (!InitMenu())
        {
            LOG_ERROR("[MacApplication]: Failed to initialize the application menu");
            return false;
        }
        
        // Create mainwindow for application
        m_pMainWindow = (MacWindow*)MacApplication::CreateWindow("Lambda Game Engine", 1440, 900);
        if (!m_pMainWindow)
        {
            return false;
        }
        
        m_pMainWindow->Show();
        
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

    void MacApplication::AddMacMessageHandler(IMacMessageHandler* pMacMessageHandler)
    {
        //Check first so that this handler is not already added
        const uint32 count = uint32(m_MessageHandlers.size());
        for (uint32 i = 0; i < count; i++)
        {
            if (pMacMessageHandler == m_MessageHandlers[i])
            {
                return;
            }
        }

        // Add new handler
        m_MessageHandlers.emplace_back(pMacMessageHandler);
    }

    void MacApplication::RemoveMacMessageHandler(IMacMessageHandler* pMacMessageHandler)
    {
        const uint32 count = uint32(m_MessageHandlers.size());
        for (uint32 i = 0; i < count; i++)
        {
            if (pMacMessageHandler == m_MessageHandlers[i])
            {
                // A handler must be unique since we check for it when adding a handler, therefore we can break here
                m_MessageHandlers.erase(m_MessageHandlers.begin() + i);
                break;
            }
        }
    }

    void MacApplication::StoreNSEvent(NSEvent* pEvent)
    {
        MacEvent storedEvent = { };
        storedEvent.pEvent = [pEvent retain];

        NSWindow* window = [pEvent window];
        if (window)
        {
            if ([window isKindOfClass:[CocoaWindow class]])
            {
                storedEvent.pEventWindow = (CocoaWindow*)[window retain];
            }
        }
        
        StoreEvent(&storedEvent);
    }

    void MacApplication::StoreEvent(const MacEvent* pEvent)
    {
        m_StoredEvents.emplace_back(*pEvent);
    }

    void MacApplication::ProcessEvent(const MacEvent* pEvent)
    {
        NSEvent*        event           = pEvent->pEvent;
        NSNotification* notification    = pEvent->pNotification;
        
        if (notification)
        {
            NSNotificationName name = [notification name];
            if (name == NSWindowWillCloseNotification)
            {
                VALIDATE(pEvent->pEventWindow != nullptr);
                
                LOG_MESSAGE("Window will close");
                
                MacWindow* pWindow = GetWindowFromNSWindow(pEvent->pEventWindow);
                if (pWindow == m_pMainWindow)
                {
                    m_IsTerminating = true;
                }
                
                for (IWindowHandler* pHandler : m_WindowHandlers)
                {
                    pHandler->WindowClosed(pWindow);
                }
            }
            else if (name == NSWindowDidMoveNotification)
            {
                VALIDATE(pEvent->pEventWindow != nullptr);
                
                MacWindow* pWindow = GetWindowFromNSWindow(pEvent->pEventWindow);
                for (IWindowHandler* pHandler : m_WindowHandlers)
                {
                    pHandler->WindowMoved(pWindow, int16(pEvent->Position.x), int16(pEvent->Position.y));
                }
            }
            else if (name == NSWindowDidResizeNotification)
            {
                VALIDATE(pEvent->pEventWindow != nullptr);
                
                MacWindow* pWindow = GetWindowFromNSWindow(pEvent->pEventWindow);
                for (IWindowHandler* pHandler : m_WindowHandlers)
                {
                    pHandler->WindowResized(pWindow, uint16(pEvent->Size.width), uint16(pEvent->Size.height), EResizeType::RESIZE_TYPE_NONE);
                }
            }
            else if (name == NSWindowDidMiniaturizeNotification)
            {
                VALIDATE(pEvent->pEventWindow != nullptr);
                
                MacWindow* pWindow = GetWindowFromNSWindow(pEvent->pEventWindow);
                for (IWindowHandler* pHandler : m_WindowHandlers)
                {
                    pHandler->WindowResized(pWindow, uint16(pEvent->Size.width), uint16(pEvent->Size.height), EResizeType::RESIZE_TYPE_MINIMIZE);
                }
            }
            else if (name == NSWindowDidDeminiaturizeNotification)
            {
                VALIDATE(pEvent->pEventWindow != nullptr);
                
                MacWindow* pWindow = GetWindowFromNSWindow(pEvent->pEventWindow);
                for (IWindowHandler* pHandler : m_WindowHandlers)
                {
                    pHandler->WindowResized(pWindow, uint16(pEvent->Size.width), uint16(pEvent->Size.height), EResizeType::RESIZE_TYPE_MAXIMIZE);
                }
            }
            else if (name == NSWindowDidBecomeKeyNotification)
            {
                VALIDATE(pEvent->pEventWindow != nullptr);
                
                MacWindow* pWindow = GetWindowFromNSWindow(pEvent->pEventWindow);
                for (IWindowHandler* pHandler : m_WindowHandlers)
                {
                    pHandler->FocusChanged(pWindow, true);
                }
            }
            else if (name == NSWindowDidResignKeyNotification)
            {
                VALIDATE(pEvent->pEventWindow != nullptr);
                
                MacWindow* pWindow = GetWindowFromNSWindow(pEvent->pEventWindow);
                for (IWindowHandler* pHandler : m_WindowHandlers)
                {
                    pHandler->FocusChanged(pWindow, false);
                }
            }
        }
        else if (event)
        {
            NSEventType type = [event type];
            if (type == NSEventTypeMouseEntered && pEvent->pEventWindow)
            {
                MacWindow* pWindow = GetWindowFromNSWindow(pEvent->pEventWindow);
                if (pWindow)
                {
                    for (IWindowHandler* pHandler : m_WindowHandlers)
                    {
                        pHandler->MouseEntered(pWindow);
                    }
                }
            }
            else if (type == NSEventTypeMouseExited && pEvent->pEventWindow)
            {
                MacWindow* pWindow = GetWindowFromNSWindow(pEvent->pEventWindow);
                if (pWindow)
                {
                    for (IWindowHandler* pHandler : m_WindowHandlers)
                    {
                        pHandler->MouseLeft(pWindow);
                    }
                }
            }
        }
        
        for (IMacMessageHandler* pHandler : m_MessageHandlers)
        {
            pHandler->HandleEvent(pEvent);
        }
    }

    MacWindow* MacApplication::GetWindowFromNSWindow(CocoaWindow* pWindow)
    {
        for (MacWindow* pMacWindow : m_Windows)
        {
            CocoaWindow* nsHandle = (CocoaWindow*)pMacWindow->GetHandle();
            if (nsHandle == pWindow)
            {
                return pMacWindow;
            }
        }
        
        return nullptr;
    }

    void MacApplication::AddWindowHandler(IWindowHandler* pHandler)
    {
        // Check first so that this handler is not already added
		const uint32 count = uint32(m_WindowHandlers.size());
		for (uint32 i = 0; i < count; i++)
		{
			if (pHandler == m_WindowHandlers[i])
			{
				return;
			}
		}

		// Add new handler
		m_WindowHandlers.emplace_back(pHandler);
    }

    void MacApplication::RemoveWindowHandler(IWindowHandler* pHandler)
    {
        const uint32 count = uint32(m_WindowHandlers.size());
		for (uint32 i = 0; i < count; i++)
		{
			if (pHandler == m_WindowHandlers[i])
			{
                // A handler must be unique since we check for it when adding a handler, therefore we can break here
				m_WindowHandlers.erase(m_WindowHandlers.begin() + i);
				break;
			}
		}
    }

    void MacApplication::ProcessStoredEvents()
    {
        m_IsProcessingEvents = true;
        
        TArray<MacEvent> eventsToProcess = TArray<MacEvent>(m_StoredEvents);
        m_StoredEvents.clear();
        
        for (const MacEvent& event : eventsToProcess)
        {
            ProcessEvent(&event);
        }
        
        m_IsProcessingEvents = false;
    }

    IWindow* MacApplication::GetForegroundWindow() const
    {
        VALIDATE(MacApplication::Get() != nullptr);
        
        NSWindow* keyWindow = [NSApp keyWindow];
        if ([keyWindow isKindOfClass:[CocoaWindow class]])
        {
            return MacApplication::Get()->GetWindowFromNSWindow((CocoaWindow*)keyWindow);
        }
        
        return nullptr;
    }

    IWindow* MacApplication::GetMainWindow() const
    {
        VALIDATE(MacApplication::Get() != nullptr);
        return MacApplication::Get()->m_pMainWindow;
    }

    void MacApplication::AddWindow(MacWindow* pWindow)
    {
        m_Windows.emplace_back(pWindow);
    }

    /*
     * Static
     */

    void MacApplication::Terminate()
    {
        [NSApp terminate:nil];
    }

    IWindow* MacApplication::CreateWindow(const char* pTitle, uint32 width, uint32 height)
    {
        MacWindow* pWindow = DBG_NEW MacWindow();
        if (!pWindow->Init(pTitle, width, height))
        {
            SAFEDELETE(pWindow);
        }
        
        VALIDATE(MacApplication::Get() != nullptr);
        
        MacApplication::Get()->AddWindow(pWindow);
        return pWindow;
    }

    IInputDevice* MacApplication::CreateInputDevice(EInputMode)
    {
        MacInputDevice* pInputDevice = DBG_NEW MacInputDevice();
        MacApplication::Get()->AddMacMessageHandler(pInputDevice);

        return pInputDevice;
    }

    bool MacApplication::PreInit()
    {
        SCOPED_AUTORELEASE_POOL();
        
        [NSApplication sharedApplication];
        
        VALIDATE(NSApp != nil);
        
        [NSApp activateIgnoringOtherApps:YES];
        [NSApp setPresentationOptions:NSApplicationPresentationDefault];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

        MacMainThread::PreInit();
        
        MacApplication* pApplication = DBG_NEW MacApplication();
        if (!pApplication->Init())
        {
            return false;
        }
        
        if (!MacInputCodeTable::Init())
        {
            return false;
        }
        
        // Process events in the queue
        ProcessMessages();
        
        [NSApp finishLaunching];
        return true;
    }

    bool MacApplication::Tick()
    {
        MacMainThread::Tick();
        
        bool shouldExit = ProcessMessages();
        MacApplication::Get()->ProcessStoredEvents();

        // We are done by updating (We have processed all events)
        [NSApp updateWindows];
        
        return shouldExit;
    }

    bool MacApplication::ProcessMessages()
    {
        SCOPED_AUTORELEASE_POOL();
        
        // Make sure this function is called on the main thread, calling from other threads result in undefined
        VALIDATE([NSThread isMainThread]);
        
        if (MacApplication::Get())
        {
            // Checking events while processing buffered events causes some events to get lost
            if (!MacApplication::Get()->m_IsProcessingEvents)
            {
                NSEvent* event = nil;
                while (true)
                {
                    event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES];
                    if (!event)
                    {
                        break;
                    }
                    
                    MacApplication::Get()->StoreNSEvent(event);
                    [NSApp sendEvent:event];
                }
            }
            
            if (MacApplication::Get()->m_IsTerminating)
            {
                return false;
            }
        }
        
        return true;
    }

    bool MacApplication::PostRelease()
    {
        SAFEDELETE(s_pApplication);
        
        MacMainThread::PostRelease();
        return true;
    }
}

#endif
