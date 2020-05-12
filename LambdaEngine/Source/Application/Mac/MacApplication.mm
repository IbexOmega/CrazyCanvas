#ifdef LAMBDA_PLATFORM_MACOS
#include "Log/Log.h"

#include "Memory/Memory.h"

#include "Application/API/IEventHandler.h"

#include "Application/Mac/MacConsole.h"
#include "Application/Mac/MacApplication.h"
#include "Application/Mac/MacScopedPool.h"
#include "Application/Mac/MacWindow.h"
#include "Application/Mac/CocoaWindow.h"
#include "Application/Mac/CocoaAppDelegate.h"
#include "Application/Mac/IMacEventHandler.h"

#include "Input/Mac/MacInputCodeTable.h"

#include "Threading/Mac/MacMainThread.h"

#include <mutex>

#include <Appkit/Appkit.h>

namespace LambdaEngine
{
	static bool InitMenu()
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
     * MacApplication
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
        MacWindow* pWindow = (MacWindow*)MacApplication::CreateWindow("Lambda Game Engine", 1440, 900);
        if (!pWindow)
        {
            return false;
        }
        
        pWindow->Show();
        MakeMainWindow(pWindow);
        
        return true;
    }

    void MacApplication::AddMacEventHandler(IMacEventHandler* pMacMessageHandler)
    {
        //Check first so that this handler is not already added
        const uint32 count = uint32(m_MacEventHandlers.size());
        for (uint32 i = 0; i < count; i++)
        {
            if (pMacMessageHandler == m_MacEventHandlers[i])
            {
                return;
            }
        }

        // Add new handler
        m_MacEventHandlers.emplace_back(pMacMessageHandler);
    }

    void MacApplication::RemoveMacEventHandler(IMacEventHandler* pMacMessageHandler)
    {
        const uint32 count = uint32(m_MacEventHandlers.size());
        for (uint32 i = 0; i < count; i++)
        {
            if (pMacMessageHandler == m_MacEventHandlers[i])
            {
                // A handler must be unique since we check for it when adding a handler, therefore we can break here
                m_MacEventHandlers.erase(m_MacEventHandlers.begin() + i);
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

	void MacApplication::ProcessNSEvent(NSEvent* pEvent)
	{
		// Store the event for later
		StoreNSEvent(pEvent);
		
		// Let all the eventhandlers know
        for (IMacEventHandler* pMacEventHandler : m_MacEventHandlers)
        {
            pMacEventHandler->HandleEvent(pEvent);
        }
	}

    void MacApplication::ProcessStoredEvent(const MacEvent* pEvent)
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
                
                for (IEventHandler* pHandler : m_EventHandlers)
                {
                    pHandler->WindowClosed(pWindow);
                }
            }
            else if (name == NSWindowDidMoveNotification)
            {
                VALIDATE(pEvent->pEventWindow != nullptr);
                
                MacWindow* pWindow = GetWindowFromNSWindow(pEvent->pEventWindow);
                for (IEventHandler* pHandler : m_EventHandlers)
                {
                    pHandler->WindowMoved(pWindow, int16(pEvent->Position.x), int16(pEvent->Position.y));
                }
            }
            else if (name == NSWindowDidResizeNotification)
            {
                VALIDATE(pEvent->pEventWindow != nullptr);
                
                MacWindow* pWindow = GetWindowFromNSWindow(pEvent->pEventWindow);
                for (IEventHandler* pHandler : m_EventHandlers)
                {
                    pHandler->WindowResized(pWindow, uint16(pEvent->Size.width), uint16(pEvent->Size.height), EResizeType::RESIZE_TYPE_NONE);
                }
            }
            else if (name == NSWindowDidMiniaturizeNotification)
            {
                VALIDATE(pEvent->pEventWindow != nullptr);
                
                MacWindow* pWindow = GetWindowFromNSWindow(pEvent->pEventWindow);
                for (IEventHandler* pHandler : m_EventHandlers)
                {
                    pHandler->WindowResized(pWindow, uint16(pEvent->Size.width), uint16(pEvent->Size.height), EResizeType::RESIZE_TYPE_MINIMIZE);
                }
            }
            else if (name == NSWindowDidDeminiaturizeNotification)
            {
                VALIDATE(pEvent->pEventWindow != nullptr);
                
                MacWindow* pWindow = GetWindowFromNSWindow(pEvent->pEventWindow);
                for (IEventHandler* pHandler : m_EventHandlers)
                {
                    pHandler->WindowResized(pWindow, uint16(pEvent->Size.width), uint16(pEvent->Size.height), EResizeType::RESIZE_TYPE_MAXIMIZE);
                }
            }
            else if (name == NSWindowDidBecomeKeyNotification)
            {
                VALIDATE(pEvent->pEventWindow != nullptr);
                
                MacWindow* pWindow = GetWindowFromNSWindow(pEvent->pEventWindow);
                for (IEventHandler* pHandler : m_EventHandlers)
                {
                    pHandler->FocusChanged(pWindow, true);
                }
            }
            else if (name == NSWindowDidResignKeyNotification)
            {
                VALIDATE(pEvent->pEventWindow != nullptr);
                
                MacWindow* pWindow = GetWindowFromNSWindow(pEvent->pEventWindow);
                for (IEventHandler* pHandler : m_EventHandlers)
                {
                    pHandler->FocusChanged(pWindow, false);
                }
            }
        }
        else if (event)
        {
            NSEventType type = [event type];
			switch(type)
			{
				case NSEventTypeKeyUp:
				{
					const uint16  macKey = [event keyCode];
					const EKey    key    = MacInputCodeTable::GetKey(macKey);

					for (IEventHandler* pHandler : m_EventHandlers)
					{
						pHandler->KeyReleased(key);
					}

					break;
				}
				   
				case NSEventTypeKeyDown:
				{
					const uint16  macKey = [event keyCode];
					const EKey    key    = MacInputCodeTable::GetKey(macKey);

					const uint32 modifierFlags  = [event modifierFlags];
					const uint32 modifiers      = MacInputCodeTable::GetModiferMask(modifierFlags);

					for (IEventHandler* pHandler : m_EventHandlers)
					{
						pHandler->KeyPressed(key, modifiers, [event isARepeat]);
					}
					
					break;
				}

				case NSEventTypeLeftMouseUp:
				case NSEventTypeRightMouseUp:
				case NSEventTypeOtherMouseUp:
				{
					const NSInteger         macButton   = [event buttonNumber];
					const EMouseButton      button      = MacInputCodeTable::GetMouseButton(int32(macButton));
					
					for (IEventHandler* pHandler : m_EventHandlers)
					{
						pHandler->ButtonReleased(button);
					}
				   
					break;
				}

				case NSEventTypeLeftMouseDown:
				case NSEventTypeRightMouseDown:
				case NSEventTypeOtherMouseDown:
				{
					const NSInteger       macButton   = [event buttonNumber];
					const EMouseButton    button      = MacInputCodeTable::GetMouseButton(int32(macButton));

					const NSUInteger modifierFlags = [event modifierFlags];
					const uint32     modifierMask  = MacInputCodeTable::GetModiferMask(modifierFlags);
				   
					for (IEventHandler* pHandler : m_EventHandlers)
					{
						pHandler->ButtonPressed(button, modifierMask);
					}
					
					break;
				}

				case NSEventTypeLeftMouseDragged:
				case NSEventTypeOtherMouseDragged:
				case NSEventTypeRightMouseDragged:
				case NSEventTypeMouseMoved:
				{
					const NSPoint mousePosition = [event locationInWindow];

					int32 x = 0;
					int32 y = 0;
					if (pEvent->pEventWindow)
					{
					   const NSRect contentRect = [pEvent->pEventWindow frame];
					   x = int32(mousePosition.x);
					   y = int32(contentRect.size.height - mousePosition.y);
					}
					else
					{
					   x = int32(mousePosition.x);
					   y = int32(mousePosition.y);
					}
					
					for (IEventHandler* pHandler : m_EventHandlers)
					{
						pHandler->MouseMoved(x, y);
					}
					
					break;
				}
				   
				case NSEventTypeScrollWheel:
				{
					CGFloat scrollDeltaX = [event scrollingDeltaX];
					CGFloat scrollDeltaY = [event scrollingDeltaY];
					if ([event hasPreciseScrollingDeltas])
					{
					   scrollDeltaX *= 0.1;
					   scrollDeltaY *= 0.1;
					}
					
					for (IEventHandler* pHandler : m_EventHandlers)
					{
						pHandler->MouseScrolled(int32(scrollDeltaX), int32(scrollDeltaY));
					}
					
					break;
				}
					
				case NSEventTypeMouseEntered:
				{
					if (pEvent->pEventWindow)
					{
						MacWindow* pWindow = GetWindowFromNSWindow(pEvent->pEventWindow);
						if (pWindow)
						{
							for (IEventHandler* pHandler : m_EventHandlers)
							{
								pHandler->MouseEntered(pWindow);
							}
						}
					}
					
					break;
				}
					
				case NSEventTypeMouseExited:
				{
					if (pEvent->pEventWindow)
					{
						MacWindow* pWindow = GetWindowFromNSWindow(pEvent->pEventWindow);
						if (pWindow)
						{
							for (IEventHandler* pHandler : m_EventHandlers)
							{
								pHandler->MouseLeft(pWindow);
							}
						}
					}
					
					break;
				}
					
				default:
				{
					break;
				}
			}
        }
        else if (pEvent->pKeyTypedText)
        {
            NSString*   text    = pEvent->pKeyTypedText;
            NSUInteger  count   = [text length];
            for (NSUInteger i = 0; i < count; i++)
            {
                // Equal to unsigned short
                const unichar codepoint = [text characterAtIndex:i];
                if ((codepoint & 0xff00) != 0xf700)
                {
					for (IEventHandler* pHandler : m_EventHandlers)
					{
						pHandler->KeyTyped(uint32(codepoint));
					}
                }
            }
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

    void MacApplication::AddEventHandler(IEventHandler* pEventHandler)
    {
        // Check first so that this handler is not already added
		const uint32 count = uint32(m_EventHandlers.size());
		for (uint32 i = 0; i < count; i++)
		{
			if (pEventHandler == m_EventHandlers[i])
			{
				return;
			}
		}

		// Add new handler
		m_EventHandlers.emplace_back(pEventHandler);
    }

    void MacApplication::RemoveEventHandler(IEventHandler* pEventHandler)
    {
        const uint32 count = uint32(m_EventHandlers.size());
		for (uint32 i = 0; i < count; i++)
		{
			if (pEventHandler == m_EventHandlers[i])
			{
                // A handler must be unique since we check for it when adding a handler, therefore we can break here
				m_EventHandlers.erase(m_EventHandlers.begin() + i);
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
            ProcessStoredEvent(&event);
        }
        
        m_IsProcessingEvents = false;
    }

    void MacApplication::MakeMainWindow(IWindow* pMainWindow)
    {
        m_pMainWindow = reinterpret_cast<MacWindow*>(pMainWindow);
    }

	void MacApplication::SetInputMode(EInputMode inputMode)
	{
		if (inputMode != EInputMode::INPUT_MODE_STANDARD)
		{
			LOG_ERROR("[MacApplication]: Unsupported inputmode");
		}
	}

	EInputMode MacApplication::GetInputMode() const
	{
		return EInputMode::INPUT_MODE_STANDARD;
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

    MacApplication* MacApplication::Get()
	{
		return s_pApplication;
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
            if (!MacApplication::Get()->IsProcessingEvents())
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
            
            if (MacApplication::Get()->IsTerminating())
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
