#include "Engine/EngineLoop.h"

#include "Log/Log.h"

#include "Platform/PlatformTime.h"
#include "Platform/PlatformMisc.h"
#include "Platform/PlatformConsole.h"

#include "Input/Input.h"

namespace LambdaEngine
{
	void EngineLoop::Run(Game* pGame)
	{
        LOG_MESSAGE("Hello World %d", 5);
        LOG_WARNING("Hello World %d", 5);
        LOG_ERROR("Hello World %d", 5);
        
		bool IsRunning = true;
		while (IsRunning)
		{
			IsRunning = Tick();
			pGame->Tick();
        }
    }

    bool EngineLoop::Tick()
    {
        if (!PlatformApplication::Tick())
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
        
        PlatformConsole::Show();
        return true;
	}
	
	bool EngineLoop::Init()
	{
		if (!Input::Init())
		{
			return false;
		}

		return true;
	}
	
	bool EngineLoop::Release()
	{
		Input::Release();

		return true;
	}
	
	bool EngineLoop::PostRelease()
	{
		if (!PlatformApplication::PostRelease())
		{
			return false;
		}

        PlatformConsole::Close();
		return true;
	}
}
