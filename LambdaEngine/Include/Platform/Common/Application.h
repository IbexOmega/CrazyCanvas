#pragma once
#include "LambdaEngine.h"

namespace LambdaEngine
{
    class Window;
    class IApplicationMessageHandler;
	class IInputDevice;

	class Application
	{
	public:
        Application()   = default;
        ~Application()  = default;
        
        DECL_REMOVE_COPY(Application);
        DECL_REMOVE_MOVE(Application);

        virtual void AddMessageHandler(IApplicationMessageHandler* pListener) = 0;
        
        virtual Window*         GetWindow()         = 0;
        virtual const Window*   GetWindow() const   = 0;
        
		static bool PreInit() 		{ return true; }
		static bool PostRelease() 	{ return true; }
		
		static bool Tick() { return false; }

		static IInputDevice* CreateInputDevice() { return nullptr; }
        
        static void Terminate() {}

        static Application* Get() { return nullptr; }
	};
}
