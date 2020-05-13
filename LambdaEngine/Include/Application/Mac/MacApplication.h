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
    class IMacEventHandler;

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
        CocoaWindow*    pEventWindow    = nullptr;
        NSEvent*        pEvent          = nullptr;
        NSNotification* pNotification   = nullptr;
        NSString*       pKeyTypedText   = nullptr;
        
        CGSize  Size;
        CGPoint Position;
    };

    /*
     * Class that represents the OS- application. Handles windows and the event loop.
     */
    class MacApplication : public Application
    {
    public:
        MacApplication();
        ~MacApplication();

        bool Init();
        
        void AddMacEventHandler(IMacEventHandler* pMacMessageHandler);
        void RemoveMacEventHandler(IMacEventHandler* pMacMessageHandler);
        
        void StoreNSEvent(NSEvent* pEvent);
        void StoreEvent(const MacEvent* pEvent);
		
		FORCEINLINE bool IsTerminating() const
		{
			return m_IsTerminating;
		}
		
		FORCEINLINE bool IsProcessingEvents() const
		{
			return m_IsProcessingEvents;
		}
        
        MacWindow* GetWindowFromNSWindow(CocoaWindow* pWindow);

        // Application
        virtual void AddEventHandler(IEventHandler* pHandler)    override final;
        virtual void RemoveEventHandler(IEventHandler* pHandler) override final;
        
        virtual void ProcessStoredEvents() override final;

        virtual void MakeMainWindow(IWindow* pMainWindow) override final;

		virtual void SetInputMode(EInputMode inputMode) override final;
		
		virtual EInputMode GetInputMode() const override final;
		
        virtual IWindow* GetForegroundWindow()   const override final;
        virtual IWindow* GetMainWindow()         const override final;
        
    private:
		void AddWindow(MacWindow* pWindow);
        
		void ProcessNSEvent(NSEvent* pEvent);
		void ProcessStoredEvent(const MacEvent* pEvent);

    public:
        static bool PreInit();
        static bool PostRelease();
        
        static bool Tick();
        static bool ProcessMessages();
        
        static void Terminate();
        
        static IWindow* CreateWindow(const char* pTitle, uint32 width, uint32 height);
        
		static MacApplication* Get();
        
    private:
        CocoaAppDelegate* m_pAppDelegate    = nullptr;
        MacWindow*        m_pMainWindow     = nullptr;
        
        bool m_IsProcessingEvents   = false;
        bool m_IsTerminating        = false;
        
        TArray<MacWindow*>          m_Windows;
        TArray<MacEvent>            m_StoredEvents;
        TArray<IEventHandler*>     	m_EventHandlers;
        TArray<IMacEventHandler*> 	m_MacEventHandlers;
        
    private:
        static MacApplication* s_pApplication;
    };

    typedef MacApplication PlatformApplication;
}

#endif
