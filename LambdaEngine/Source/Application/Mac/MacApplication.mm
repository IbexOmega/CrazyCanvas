#ifdef LAMBDA_PLATFORM_MACOS
#include "Log/Log.h"

#include "Memory/API/Memory.h"

#include "Application/API/EventHandler.h"

#include "Application/Mac/MacConsole.h"
#include "Application/Mac/MacApplication.h"
#include "Application/Mac/MacScopedPool.h"
#include "Application/Mac/MacWindow.h"
#include "Application/Mac/CocoaWindow.h"
#include "Application/Mac/CocoaAppDelegate.h"

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

		// Destroy windows
		for (MacWindow* pWindow : m_Windows)
		{
			SAFERELEASE(pWindow);
		}

		[m_pAppDelegate release];
		
		VALIDATE_MSG(s_pApplication != nullptr, "[MacApplication]: Instance of application has already been deleted");
		s_pApplication = nullptr;
		
		// Release mainthread
		MacMainThread::PostRelease();
	}

	void MacApplication::StoreNSEvent(NSEvent* pEvent)
	{
		MacEvent event 	= { };
		event.pEvent = [pEvent retain];

		NSWindow* window = [pEvent window];
		if (window)
		{
			if ([window isKindOfClass:[CocoaWindow class]])
			{
				event.pEventWindow = reinterpret_cast<CocoaWindow*>([window retain]);
			}
		}
		
		StoreEvent(event);
	}

	void MacApplication::StoreEvent(const MacEvent& event)
	{
		m_StoredEvents.emplace_back(event);
	}

	bool MacApplication::ProcessStoredEvent(const MacEvent& event)
	{
		NSEvent*		nsEvent			= event.pEvent;
		NSNotification*	notification	= event.pNotification;
		
		if (notification)
		{
			NSNotificationName name = [notification name];
			if (name == NSWindowWillCloseNotification)
			{
				VALIDATE(event.pEventWindow != nullptr);
				
				MacWindow* pWindow = GetWindowFromNSWindow(event.pEventWindow);
				m_pEventHandler->OnWindowClosed(pWindow);
			}
			else if (name == NSWindowDidMoveNotification)
			{
				VALIDATE(event.pEventWindow != nullptr);
				
				MacWindow* pWindow = GetWindowFromNSWindow(event.pEventWindow);
				m_pEventHandler->OnWindowMoved(pWindow, int16(event.Position.x), int16(event.Position.y));
			}
			else if (name == NSWindowDidResizeNotification)
			{
				VALIDATE(event.pEventWindow != nullptr);
				
				MacWindow* pWindow = GetWindowFromNSWindow(event.pEventWindow);
				m_pEventHandler->OnWindowResized(pWindow, uint16(event.Size.width), uint16(event.Size.height), EResizeType::RESIZE_TYPE_NONE);
			}
			else if (name == NSWindowDidMiniaturizeNotification)
			{
				VALIDATE(event.pEventWindow != nullptr);
				
				MacWindow* pWindow = GetWindowFromNSWindow(event.pEventWindow);
				m_pEventHandler->OnWindowResized(pWindow, uint16(event.Size.width), uint16(event.Size.height), EResizeType::RESIZE_TYPE_MINIMIZE);
			}
			else if (name == NSWindowDidDeminiaturizeNotification)
			{
				VALIDATE(event.pEventWindow != nullptr);
				
				MacWindow* pWindow = GetWindowFromNSWindow(event.pEventWindow);
				m_pEventHandler->OnWindowResized(pWindow, uint16(event.Size.width), uint16(event.Size.height), EResizeType::RESIZE_TYPE_MAXIMIZE);
			}
			else if (name == NSWindowDidBecomeKeyNotification)
			{
				VALIDATE(event.pEventWindow != nullptr);
				
				MacWindow* pWindow = GetWindowFromNSWindow(event.pEventWindow);
				m_pEventHandler->OnFocusChanged(pWindow, true);
			}
			else if (name == NSWindowDidResignKeyNotification)
			{
				VALIDATE(event.pEventWindow != nullptr);
				
				MacWindow* pWindow = GetWindowFromNSWindow(event.pEventWindow);
				m_pEventHandler->OnFocusChanged(pWindow, false);
			}
			else if (name == NSApplicationWillTerminateNotification)
			{
				m_IsTerminating = true;
				return false;
			}
		}
		else if (nsEvent)
		{
			NSEventType type = [nsEvent type];
			switch(type)
			{
				case NSEventTypeKeyUp:
				{
					const uint16  macKey = [nsEvent keyCode];
					const EKey    key    = MacInputCodeTable::GetKey(macKey);

					m_pEventHandler->OnKeyReleased(key);
					break;
				}
				   
				case NSEventTypeKeyDown:
				{
					const uint16	macKey	= [nsEvent keyCode];
					const EKey		key		= MacInputCodeTable::GetKey(macKey);

					const uint32 modifierFlags	= [nsEvent modifierFlags];
					const uint32 modifiers		= MacInputCodeTable::GetModiferMask(modifierFlags);

					m_pEventHandler->OnKeyPressed(key, modifiers, [nsEvent isARepeat]);
					break;
				}

				case NSEventTypeLeftMouseUp:
				case NSEventTypeRightMouseUp:
				case NSEventTypeOtherMouseUp:
				{
					const NSInteger		macButton	= [nsEvent buttonNumber];
					const EMouseButton	button		= MacInputCodeTable::GetMouseButton(int32(macButton));
						
					m_pEventHandler->OnButtonReleased(button);
					break;
				}

				case NSEventTypeLeftMouseDown:
				case NSEventTypeRightMouseDown:
				case NSEventTypeOtherMouseDown:
				{
					const NSInteger		macButton	= [nsEvent buttonNumber];
					const EMouseButton	button		= MacInputCodeTable::GetMouseButton(int32(macButton));

					const NSUInteger	modifierFlags	= [nsEvent modifierFlags];
					const uint32		modifierMask	= MacInputCodeTable::GetModiferMask(modifierFlags);

					m_pEventHandler->OnButtonPressed(button, modifierMask);
					break;
				}

				case NSEventTypeLeftMouseDragged:
				case NSEventTypeOtherMouseDragged:
				case NSEventTypeRightMouseDragged:
				case NSEventTypeMouseMoved:
				{
					if (event.pEventWindow)
					{
						const NSPoint	mousePosition	= [nsEvent locationInWindow];
						const NSRect	contentRect 	= [event.pEventWindow frame];
						
						const int32 x = int32(mousePosition.x);
						const int32 y = int32(contentRect.size.height - mousePosition.y);
						
						m_pEventHandler->OnMouseMoved(x, y);
						break;
					}
				}
				   
				case NSEventTypeScrollWheel:
				{
					CGFloat scrollDeltaX = [nsEvent scrollingDeltaX];
					CGFloat scrollDeltaY = [nsEvent scrollingDeltaY];
					if ([nsEvent hasPreciseScrollingDeltas])
					{
						scrollDeltaX *= 0.1;
						scrollDeltaY *= 0.1;
					}
						
					m_pEventHandler->OnMouseScrolled(int32(scrollDeltaX), int32(scrollDeltaY));
					break;
				}
					
				case NSEventTypeMouseEntered:
				{
					MacWindow* pWindow = GetWindowFromNSWindow(event.pEventWindow);
					if (pWindow)
					{
						m_pEventHandler->OnMouseEntered(pWindow);
					}

					break;
				}
					
				case NSEventTypeMouseExited:
				{
					MacWindow* pWindow = GetWindowFromNSWindow(event.pEventWindow);
					if (pWindow)
					{
						m_pEventHandler->OnMouseLeft(pWindow);
					}

					break;
				}
					
				default:
				{
					break;
				}
			}
		}
		else if (event.pKeyTypedText)
		{
			NSString*	text	= event.pKeyTypedText;
			NSUInteger	count	= [text length];
			for (NSUInteger i = 0; i < count; i++)
			{
				// Equal to unsigned short
				const unichar codepoint = [text characterAtIndex:i];
				if ((codepoint & 0xff00) != 0xf700)
				{
					m_pEventHandler->OnKeyTyped(uint32(codepoint));
				}
			}
		}

		return true;
	}

	MacWindow* MacApplication::GetWindowFromNSWindow(CocoaWindow* pWindow) const
	{
		for (MacWindow* pMacWindow : m_Windows)
		{
			CocoaWindow* nsHandle = reinterpret_cast<CocoaWindow*>(pMacWindow->GetHandle());
			if (nsHandle == pWindow)
			{
				return pMacWindow;
			}
		}
		
		return nullptr;
	}

	bool MacApplication::Create()
	{
		SCOPED_AUTORELEASE_POOL();
		
		[NSApplication sharedApplication];
		
		VALIDATE(NSApp != nullptr);
		
		[NSApp activateIgnoringOtherApps:YES];
		[NSApp setPresentationOptions:NSApplicationPresentationDefault];
		[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
		
		m_pAppDelegate = [[CocoaAppDelegate alloc] init];
		[NSApp setDelegate:m_pAppDelegate];
		
		MacMainThread::PreInit();
		
		if (!MacInputCodeTable::Init())
		{
			return false;
		}

		if (!InitMenu())
		{
			LOG_ERROR("[MacApplication]: Failed to initialize the application menu");
			return false;
		}

		[NSApp finishLaunching];
		return true;
	}

	Window* MacApplication::CreateWindow(const WindowDesc* pDesc)
	{
		MacWindow* pWindow = DBG_NEW MacWindow();
		if (!pWindow->Init(pDesc))
		{
			SAFEDELETE(pWindow);
			return nullptr;
		}
		else
		{
			AddWindow(pWindow);
			return pWindow;
		}
	}

	bool MacApplication::ProcessStoredEvents()
	{
		m_IsProcessingEvents = true;
		
		TArray<MacEvent> eventsToProcess = TArray<MacEvent>(m_StoredEvents);
		m_StoredEvents.clear();
		
		bool shouldRun = true;
		for (const MacEvent& event : eventsToProcess)
		{
			if (!ProcessStoredEvent(event))
			{
				shouldRun = false;
			}
		}
		
		m_IsProcessingEvents = false;
		
		// We are done by updating (We have processed all events)
		[NSApp updateWindows];
		
		return shouldRun;
	}

	bool MacApplication::Tick()
	{
		bool shouldExit = ProcessStoredEvents();

		// Also process the mainthread
		MacMainThread::Tick();
		return shouldExit;
	}

	bool MacApplication::SupportsRawInput() const
	{
		return false;
	}

	void MacApplication::SetInputMode(Window* pWindow, EInputMode inputMode)
	{
		UNREFERENCED_VARIABLE(pWindow);
		
		if (inputMode != EInputMode::INPUT_MODE_STANDARD)
		{
			LOG_ERROR("[MacApplication]: Unsupported inputmode");
		}
	}

	EInputMode MacApplication::GetInputMode(Window* pWindow) const
	{
		UNREFERENCED_VARIABLE(pWindow);
		return EInputMode::INPUT_MODE_STANDARD;
	}

	void MacApplication::SetActiveWindow(Window *pWindow)
	{
		CocoaWindow* pCocoaWindow = reinterpret_cast<CocoaWindow*>(pWindow->GetHandle());
		[pCocoaWindow makeKeyAndOrderFront:pCocoaWindow];
	}

	Window* MacApplication::GetActiveWindow() const
	{
		NSWindow* keyWindow = [NSApp keyWindow];
		if ([keyWindow isKindOfClass:[CocoaWindow class]])
		{
			CocoaWindow* nsHandle = reinterpret_cast<CocoaWindow*>(keyWindow);
			return GetWindowFromNSWindow(nsHandle);
		}
		
		return nullptr;
	}

	void MacApplication::AddWindow(MacWindow* pWindow)
	{
		m_Windows.emplace_back(pWindow);
	}

	void MacApplication::Terminate()
	{
		[NSApp terminate:nil];
	}

	Application* MacApplication::CreateApplication()
	{
		return DBG_NEW MacApplication();
	}

	MacApplication* MacApplication::Get()
	{
		VALIDATE(s_pApplication != nullptr);
		return s_pApplication;
	}

	bool MacApplication::PeekEvents()
	{
		SCOPED_AUTORELEASE_POOL();
		
		// Make sure this function is called on the main thread, calling from other threads result in undefined
		VALIDATE([NSThread isMainThread]);
		
		if (s_pApplication)
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
					
					[NSApp sendEvent:event];
				}
			}
		}
		
		return true;
	}
}

#endif
