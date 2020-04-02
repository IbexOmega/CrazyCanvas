#pragma once

#ifdef LAMBDA_PLATFORM_MACOS
#include <vector>

#include "Application/API/Application.h"

#include "MacWindow.h"

#ifdef __OBJC__

@class NSEvent;
@class NSWindow;
@class NSNotification;
@class MacAppController;

#else

class NSEvent;
class NSWindow;
class NSNotification;
class MacAppController;

#endif

namespace LambdaEngine
{
    struct MacMessage
    {
        NSEvent*        event           = nullptr;
        NSNotification* notification    = nullptr;
    };

    class MacApplication : public Application
    {
    public:
        MacApplication() = default;
        ~MacApplication();

        bool Init();
        bool InitMenu();
        
        void BufferEvent(NSEvent* event);

        virtual void AddMessageHandler(IApplicationMessageHandler* pHandler)    override final;
        virtual void RemoveMessageHandler(IApplicationMessageHandler* pHandler) override final;
        
        virtual void ProcessBufferedMessages() override final;

        virtual Window*         GetWindow()         override final;
        virtual const Window*   GetWindow() const   override final;
        
    public:
        static bool PreInit();
        static bool PostRelease();
        
        static bool Tick();
        static bool ProcessMessages();
        
        static void Terminate();
        
        static Window*      CreateWindow(const char* pTitle, uint32 width, uint32 height);
        static InputDevice* CreateInputDevice(EInputMode inputMode);
        
        FORCEINLINE static MacApplication* Get()
        {
            return s_pApplication;
        }
        
    private:
        MacWindow*        m_pWindow         = nullptr;
        MacAppController* m_pAppDelegate    = nullptr;
        bool              m_IsTerminating   = false;
        
        std::vector<MacMessage>                     m_BufferedMessages;
        std::vector<IApplicationMessageHandler*>    m_MessageHandlers;

        static MacApplication* s_pApplication;
    };

    typedef MacApplication PlatformApplication;
}

#endif
