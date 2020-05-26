#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#include "Containers/TArray.h"

#include "Application/API/Application.h"

#include <CoreGraphics/CoreGraphics.h>

#ifdef __OBJC__
@class NSEvent;
@class NSNotification;
@class NSString;
@class CocoaWindow;
@class CocoaAppDelegate;
#else
class NSEvent;
class NSNotification;
class NSString;
class CocoaWindow;
class CocoaAppDelegate;
#endif

namespace LambdaEngine
{
	class MacWindow;

	/*
	* Struct used to buffer events from the OS
	*/
	struct MacEvent
	{
	public:
		MacEvent();
		MacEvent(const MacEvent& other);
		~MacEvent();
		
	public:
		CocoaWindow*	pEventWindow	= nullptr;
		NSEvent*		pEvent			= nullptr;
		NSNotification*	pNotification	= nullptr;
		NSString*		pKeyTypedText	= nullptr;
		
		CGSize  Size;
		CGPoint Position;
	};

	/*
	 * Class that represents the OS- application. Handles windows and the event loop.
	 */
	class MacApplication : public Application
	{
	public:		
		void StoreNSEvent(NSEvent* pEvent);
		void StoreEvent(const MacEvent& event);
		
		FORCEINLINE bool IsTerminating() const
		{
			return m_IsTerminating;
		}
		
		FORCEINLINE bool IsProcessingEvents() const
		{
			return m_IsProcessingEvents;
		}
		
		MacWindow* GetWindowFromNSWindow(CocoaWindow* pWindow) const;

	public:
		// Application Interface
		virtual bool	Create() 								override final;
		virtual Window*	CreateWindow(const WindowDesc* pDesc) 	override final;
		
		virtual bool Tick() override final;
		
		virtual bool ProcessStoredEvents() override final;

		virtual void Terminate() override final;
		
		virtual bool SupportsRawInput() const override final;

		virtual void 		SetInputMode(Window* pWindow, EInputMode inputMode) override final;
		virtual EInputMode	GetInputMode(Window* pWindow) const 				override final;

		virtual void 	SetActiveWindow(Window* pWindow) 	override final;
		virtual Window* GetActiveWindow() const 			override final;
		
	private:
		MacApplication();
		~MacApplication();

		void AddWindow(MacWindow* pWindow);

		void ProcessStoredEvent(const MacEvent& event);

	public:
		static bool PeekEvents();

		static Application* 	CreateApplication();
		static MacApplication* 	Get();
		
	private:
		CocoaAppDelegate* 	m_pAppDelegate	= nullptr;
		
		bool m_IsProcessingEvents   = false;
		bool m_IsTerminating        = false;
		
		TArray<MacWindow*>	m_Windows;
		TArray<MacEvent>	m_StoredEvents;
		
	private:
		static MacApplication* s_pApplication;
	};

	typedef MacApplication PlatformApplication;
}

#endif
