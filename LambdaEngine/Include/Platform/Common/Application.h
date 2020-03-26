#pragma once
#include "LambdaEngine.h"

namespace LambdaEngine
{
    class Window;
    class IApplicationMessageHandler;

	class Application
	{
	public:
        Application()   = default;
        ~Application()  = default;
        
        DECL_REMOVE_COPY(Application);
        DECL_REMOVE_MOVE(Application);

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
        
        static Application* Get() { return nullptr; }
	};
}
