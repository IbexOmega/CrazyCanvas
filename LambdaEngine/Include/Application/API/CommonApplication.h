#pragma once
#include "Application.h"

namespace LambdaEngine
{
	class LAMBDA_API CommonApplication
	{
	public:
		DECL_UNIQUE_CLASS(CommonApplication);
		
		CommonApplication();
		~CommonApplication();
		
	public:
		static bool PreInit();
		static bool PostRelease();
		
        /*
        * Application ticks one frame, processes OS- events and then processes the buffered events
        *   return - Returns false if the OS- sent a quit message. Happens when terminate is called.
        */
		static bool Tick();
        
        /*
        * Sends a quit message to the application
        */
		static void Terminate();

		static CommonApplication* Get();
		
	private:
		Application* m_pPlatformApplication = nullptr;
		
	private:
		static CommonApplication* s_pCommonApplication;
	};
}
