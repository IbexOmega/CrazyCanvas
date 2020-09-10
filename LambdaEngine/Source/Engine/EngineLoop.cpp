#include "Engine/EngineLoop.h"

#include "Log/Log.h"

#include "Time/API/PlatformTime.h"
#include "Time/API/Clock.h"

#include "Math/Random.h"

#include "Application/API/PlatformMisc.h"
#include "Application/API/PlatformConsole.h"
#include "Application/API/CommonApplication.h"

#include "Engine/EngineConfig.h"

#include "Input/API/Input.h"

#include "Networking/API/PlatformNetworkUtils.h"

#include "Threading/API/Thread.h"
#include "Threading/API/ThreadPool.h"

#include "Resources/ResourceLoader.h"
#include "Resources/ResourceManager.h"

#include "Audio/AudioSystem.h"

#include "Rendering/RenderSystem.h"
#include "Rendering/Renderer.h"

#include <assimp/Importer.hpp>

#include "Utilities/RuntimeStats.h"

#include "Game/GameConsole.h"

namespace LambdaEngine
{
	/*
	* Global clock variables
	*/
	static Clock g_Clock;
	static Timestamp g_FixedTimestep = Timestamp::Seconds(1.0 / 60.0);

	/*
	* EngineLoop
	*/
	void EngineLoop::Run()
	{
		Clock fixedClock;
		Timestamp accumulator = Timestamp(0);

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
			while (accumulator >= g_FixedTimestep)
			{
				fixedClock.Tick();
				FixedTick(fixedClock.GetDeltaTime());

				accumulator -= g_FixedTimestep;
			}
		}
	}

	bool EngineLoop::Tick(Timestamp delta)
	{
		RuntimeStats::SetFrameTime((float32)delta.AsSeconds());
		Input::Tick();

		GameConsole::Get().Tick();

		Thread::Join();

		PlatformNetworkUtils::Tick(delta);

		if (!CommonApplication::Get()->Tick())
		{
			return false;
		}

		AudioSystem::Tick();

		// Tick game
		Game::Get().Tick(delta);

		return true;
	}

	void EngineLoop::FixedTick(Timestamp delta)
	{
		// Tick game
		Game::Get().FixedTick(delta);

		NetworkUtils::FixedTick(delta);
	}

	bool EngineLoop::PreInit()
	{
#ifdef LAMBDA_DEVELOPMENT
		PlatformConsole::Show();

		Log::SetDebuggerOutputEnabled(true);

		Malloc::SetDebugFlags(MEMORY_DEBUG_FLAGS_OVERFLOW_PROTECT | MEMORY_DEBUG_FLAGS_LEAK_CHECK);
#endif

		if (!EngineConfig::LoadFromFile())
		{
			return false;
		}

		if (!ThreadPool::Init())
		{
			return false;
		}

		if (!CommonApplication::PreInit())
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

		if (!GameConsole::Get().Init())
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

		if (!Renderer::Init())
		{
			return false;
		}

		return true;
	}

	bool EngineLoop::PreRelease()
	{
		RenderSystem::GetGraphicsQueue()->Flush();
		RenderSystem::GetComputeQueue()->Flush();
		RenderSystem::GetCopyQueue()->Flush();

		return true;
	}

	bool EngineLoop::Release()
	{
		Input::Release();

		if (!GameConsole::Get().Release())
		{
			return false;
		}

		if (!Renderer::Release())
		{
			return false;
		}

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

		return ThreadPool::Release();
	}

	bool EngineLoop::PostRelease()
	{
		Thread::Release();

		PlatformNetworkUtils::Release();

		if (!CommonApplication::PostRelease())
		{
			return false;
		}

#ifdef LAMBDA_DEVELOPMENT
		PlatformConsole::Close();
#endif
		return true;
	}

	void EngineLoop::SetFixedTimestep(Timestamp timestep)
	{
		g_FixedTimestep = timestep;
	}

	Timestamp EngineLoop::GetFixedTimestep()
	{
		return g_FixedTimestep;
	}

	Timestamp EngineLoop::GetDeltaTime()
	{
		return g_Clock.GetDeltaTime();
	}

	Timestamp EngineLoop::GetTimeSinceStart()
	{
		return g_Clock.GetTotalTime();
	}
}
