#include "LambdaEngine.h"

#include "Engine/EngineLoop.h"

#include <stdio.h>

namespace LambdaEngine
{
	bool EngineLoop::s_IsRunning = false;

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
		
		return true;
	}
	
	bool EngineLoop::Init()
	{
		return true;
	}
	
	void EngineLoop::Run()
	{
		s_IsRunning = true;
		while (s_IsRunning)
		{
			s_IsRunning = Tick();
		}
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
