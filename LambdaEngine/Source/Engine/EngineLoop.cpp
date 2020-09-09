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

#include "Resources/ResourceLoader.h"
#include "Resources/ResourceManager.h"

#include "Audio/AudioSystem.h"

#include "Rendering/RenderSystem.h"
#include "Rendering/Renderer.h"

#include <assimp/Importer.hpp>

#include "Utilities/RuntimeStats.h"

#include "Application/API/Events/EventQueue.h"
#include "Application/API/Events/KeyEvents.h"

namespace LambdaEngine
{
	/*
	* Global clock variables
	*/
	static Clock g_Clock;
	static Timestamp g_FixedTimestep = Timestamp::Seconds(1.0 / 60.0);

	class A
	{
	public:
		bool HandleEvent(const Event& event)
		{
			return DispatchEvent<KeyReleasedEvent>(event, &A::HandleKey, this);
		}

		bool HandleKey(const KeyReleasedEvent& event)
		{
			LOG_INFO("Member Key event: %s", event.ToString().c_str());
			return true;
		}
	};

	static bool HandleKey(const KeyReleasedEvent& event)
	{
		LOG_INFO("Func Key event: %s", event.ToString().c_str());
		return true;
	}

	static bool HandleEvent(const Event& event)
	{
		return DispatchEvent<KeyReleasedEvent>(event, HandleKey);
	}

	/*
	* EngineLoop
	*/
	void EngineLoop::Run()
	{
		Clock fixedClock;
		Timestamp accumulator = Timestamp(0);

		g_Clock.Reset();

		A a;
		EventQueue::RegisterEventHandler(EventHandlerProxy(&a, &A::HandleEvent));
		EventQueue::RegisterEventHandler(EventHandlerProxy(&HandleEvent));

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
		RuntimeStats::SetFrameTime((float)delta.AsSeconds());
		Input::Tick();

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

		return true;
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
