#include "Engine/EngineLoop.h"

#include "Log/Log.h"

#include "Time/API/PlatformTime.h"
#include "Time/API/Clock.h"

#include "Application/API/PlatformMisc.h"
#include "Application/API/PlatformConsole.h"

#include "Input/API/Input.h"

#include "Rendering/Core/API/IGraphicsDevice.h"
#include "Rendering/Core/API/ITopLevelAccelerationStructure.h"
#include "Rendering/Core/API/IBottomLevelAccelerationStructure.h"

#include "Network/API/PlatformNetworkUtils.h"

#include "Threading/Thread.h"

#include "Resources/ResourceLoader.h"
#include "Resources/ResourceManager.h"

#include "Audio/AudioSystem.h"

#include "Rendering/RenderSystem.h"

namespace LambdaEngine
{
	void EngineLoop::Run(Game* pGame)
	{
		Clock clock;
        
        bool IsRunning = true;
        while (IsRunning)
        {
			clock.Tick();
            
			Timestamp dt = clock.GetDeltaTime();
            IsRunning = Tick(dt);
            pGame->Tick(dt);
        }
    }

    bool EngineLoop::Tick(Timestamp dt)
    {
		Thread::Join();
		PlatformNetworkUtils::Tick(dt);

        if (!PlatformApplication::Tick())
        {
            return false;
        }

		AudioSystem::Tick();

        return true;
	}

#ifdef LAMBDA_PLATFORM_WINDOWS
	bool EngineLoop::PreInit(HINSTANCE hInstance)
#else
	bool EngineLoop::PreInit()
#endif
	{
#ifndef LAMBDA_PRODUCTION
        PlatformConsole::Show();
        Log::SetDebuggerOutputEnabled(true);
#endif

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
		Thread::Init();

		if (!Input::Init())
		{
			return false;
		}

		if (!PlatformNetworkUtils::Init())
		{
			return false;
		}

		if (!RenderSystem::Init())
		{
			return false;
		}

		if (!AudioSystem::Init())
		{
			return false;
		}

		return true;
	}
	
	bool EngineLoop::Release()
	{
		Input::Release();

		if (!RenderSystem::Release())
		{
			return false;
		}

		if (!AudioSystem::Release())
		{
			return false;
		}

		return true;
	}
	
	bool EngineLoop::PostRelease()
	{
		if (!PlatformApplication::PostRelease())
		{
			return false;
		}

		Thread::Release();
		PlatformNetworkUtils::Release();

#ifndef LAMBDA_PRODUCTION
        PlatformConsole::Close();
#endif
		return true;
	}
}
