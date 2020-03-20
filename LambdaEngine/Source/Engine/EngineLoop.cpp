#include "LambdaEngine.h"

#include "Engine/EngineLoop.h"

namespace LambdaEngine
{
	bool EngineLoop::s_IsRunning = false;

	void EngineLoop::Tick()
	{
		PlatformApplication::Tick();
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
	
	bool EngineLoop::Run()
	{
		s_IsRunning = true;
		while (s_IsRunning)
		{
			Tick();
		}

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