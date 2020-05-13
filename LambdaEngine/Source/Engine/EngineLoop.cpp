#include "Engine/EngineLoop.h"

#include "Log/Log.h"

#include "Time/API/PlatformTime.h"
#include "Time/API/Clock.h"

#include "Math/Random.h"

#include "Application/API/PlatformMisc.h"
#include "Application/API/PlatformConsole.h"

#include "Input/API/Input.h"

#include "Networking/API/PlatformNetworkUtils.h"

#include "Threading/API/Thread.h"

#include "Resources/ResourceLoader.h"
#include "Resources/ResourceManager.h"

#include "Audio/AudioSystem.h"

#include "Rendering/RenderSystem.h"

namespace LambdaEngine
{
    static Clock g_Clock;

	void EngineLoop::Run()
	{
        Clock			fixedClock;
        const Timestamp timestep	= Timestamp::Seconds(1.0 / 60.0);
        Timestamp		accumulator = Timestamp(0);
        
        g_Clock.Reset();
        
        bool isRunning = true;
        while (isRunning)
        {
			g_Clock.Tick();
            
            // Update
			Timestamp delta = g_Clock.GetDeltaTime();
            isRunning = Tick(delta);
            
            // Fixed update
            accumulator += delta;
            while (accumulator >= timestep)
            {
                fixedClock.Tick();
                FixedTick(fixedClock.GetDeltaTime());
                
                accumulator -= timestep;
            }
        }
    }

    bool EngineLoop::Tick(Timestamp delta)
    {
		Input::Tick();

		Thread::Join();
        
		PlatformNetworkUtils::Tick(delta);

        if (!PlatformApplication::Tick())
        {
            return false;
        }

		AudioSystem::Tick();

        // Tick game
        Game::Get()->Tick(delta);
        
        return true;
	}

    void EngineLoop::FixedTick(Timestamp delta)
    {
        // Tick game
        Game::Get()->FixedTick(delta);
        
		NetworkUtils::FixedTick(delta);
    }

#ifdef LAMBDA_PLATFORM_WINDOWS
	bool EngineLoop::PreInit(HINSTANCE hInstance)
#else
	bool EngineLoop::PreInit()
#endif
	{
#ifdef LAMBDA_DEVELOPMENT
        PlatformConsole::Show();

        Log::SetDebuggerOutputEnabled(true);

		MemoryAllocator::SetDebugFlags(MEMORY_DEBUG_FLAGS_OVERFLOW_PROTECT | MEMORY_DEBUG_FLAGS_LEAK_CHECK);
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
              
		Random::PreInit();

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

		if (!ResourceLoader::Init())
		{
			return false;
		}

		if (!ResourceManager::Init())
		{
			return false;
		}

		return true;
	}
	
	bool EngineLoop::Release()
	{
		Input::Release();

		if (!ResourceManager::Release())
		{
			return false;
		}

		if (!ResourceLoader::Release())
		{
			return false;
		}

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
        Thread::Release();
        
        PlatformNetworkUtils::Release();
        
        if (!PlatformApplication::PostRelease())
        {
            return false;
        }

#ifndef LAMBDA_PRODUCTION
        PlatformConsole::Close();
#endif
		return true;
    }

    Timestamp EngineLoop::GetTimeSinceStart()
    {
        return g_Clock.GetTotalTime();
    }
}
