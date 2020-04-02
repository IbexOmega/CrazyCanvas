#pragma once
#include "LambdaEngine.h"

namespace LambdaEngine
{
    class Window;
	class InputDevice;
    class IApplicationMessageHandler;

    //Different input devices that can be created
    enum class EInputMode
    {
        INPUT_NONE      = 0, 
        INPUT_RAW       = 1, //Raw input on supported platforms
        INPUT_STANDARD  = 2, //Standard input from the applications event-loop
    };

	class Application
	{
	public:
        DECL_ABSTRACT_CLASS(Application);

        virtual void AddMessageHandler(IApplicationMessageHandler* pHandler)    = 0;
        virtual void RemoveMessageHandler(IApplicationMessageHandler* pHandler) = 0;
        
        virtual void ProcessBufferedMessages() = 0; 

        virtual Window*         GetWindow()         = 0;
        virtual const Window*   GetWindow() const   = 0;
        
		static bool PreInit() 		{ return true; }
		static bool PostRelease() 	{ return true; }
		
		static bool Tick()              { return false; }
        static bool ProcessMessages()   { return false; }
        
        static void Terminate() { }

        static Window*      CreateWindow(const char*, uint32, uint32)   { return nullptr; }
		static InputDevice* CreateInputDevice(EInputMode)               { return nullptr; }

        static Application* Get() { return nullptr; }
	};
}
