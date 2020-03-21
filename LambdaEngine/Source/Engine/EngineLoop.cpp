#include "Engine/EngineLoop.h"

#include "Platform/PlatformTime.h"
#include "Platform/PlatformMisc.h"

#include <iostream>

namespace LambdaEngine
{
	void EngineLoop::Run()
	{
		bool IsRunning = true;
		while (IsRunning)
		{
			IsRunning = Tick();
		}
	}

	bool EngineLoop::Tick()
	{
        if (PlatformApplication::Tick() == false)
        {
            return false;
        }
        
        return true;
	}

#ifdef LAMBDA_PLATFORM_WINDOWS
	bool EngineLoop::PreInit(HINSTANCE hInstance)
#else
	bool EngineLoop::PreInit()
#endif
	{
#ifdef LAMBDA_PLATFORM_WINDOWS
		if (!PlatformApplication::PreInit(hInstance))
#else
		if (!PlatformApplication::PreInit())
#endif
		{
			return false;
		}
		
		PlatformTime::PreInit();

		return true;
	}
	
	bool EngineLoop::Init()
	{
		return true;
	}
	
	bool EngineLoop::Release()
	{
		return true;
	}
	
	bool EngineLoop::PostRelease()
	{
		if (!PlatformApplication::PostRelease())
		{
			return false;
		}

		return true;
	}
}
