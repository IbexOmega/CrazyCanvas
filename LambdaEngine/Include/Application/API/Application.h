#pragma once
#include "LambdaEngine.h"

namespace LambdaEngine
{
    class IWindow;
	class IInputDevice;
    class IEventHandler;

    // Different input devices that can be created
    enum class EInputMode
    {
        INPUT_MODE_NONE      = 0, 
        INPUT_MODE_RAW       = 1, // Raw input on supported platforms
        INPUT_MODE_STANDARD  = 2, // Standard input from the applications event-loop
    };

	class LAMBDA_API Application
	{
	public:
        DECL_ABSTRACT_CLASS(Application);

        virtual void AddEventHandler(IEventHandler* pEventHandler)    = 0;
        virtual void RemoveEventHandler(IEventHandler* pEventHandler) = 0;

        /*
        * Application buffers all OS-events, and gets processed in a batch with this function
        */
        virtual void ProcessStoredEvents() = 0; 

        /*
        * Sets the window to be the current main window, this is not the same as the window that has
        * current focus, that is the foreground window
        *   pMainWindow - New main window
        */
        virtual void MakeMainWindow(IWindow* pMainWindow) = 0;

        /*
        * Sets the input mode for the main window
        */
        virtual void SetInputMode(EInputMode inputMode) = 0;
        
        virtual EInputMode GetInputMode() const = 0;

        virtual IWindow* GetForegroundWindow()   const = 0;
        virtual IWindow* GetMainWindow()         const = 0;
        
    public:
		static bool PreInit() 		{ return true; }
		static bool PostRelease() 	{ return true; }
		
        /*
        * Application ticks one frame, processes OS- events and then processes the buffered events
        *   return - Returns false if the OS- sent a quit message. Happens when terminate is called. 
        */
		static bool Tick() { return false; }

        /*
        * Processes all event from the OS and bufferes them up
        *   return - Returns false if the OS- sent a quit message
        */
        static bool ProcessMessages() { return false; }
        
        /*
        * Sends a quit message to the application
        */
        static void Terminate() { }

        static IWindow* 	CreateWindow(const char*, uint32, uint32)	{ return nullptr; }
		static Application* CreateApplication()							{ return nullptr; }
		
        static Application* Get() { return nullptr; }
	};
}
