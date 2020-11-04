#include "Engine/EngineLoop.h"

#include "Log/Log.h"

#include "Time/API/PlatformTime.h"
#include "Time/API/Clock.h"

#include "Application/API/PlatformMisc.h"
#include "Application/API/PlatformConsole.h"
#include "Application/API/CommonApplication.h"

#include "Application/API/Events/EventQueue.h"

#include "Math/Random.h"

#include "ECS/ECSCore.h"

#include "Engine/EngineConfig.h"

#include "Input/API/Input.h"
#include "Input/API/InputActionSystem.h"

#include "Networking/API/PlatformNetworkUtils.h"

#include "Threading/API/Thread.h"
#include "Threading/API/ThreadPool.h"

#include "Rendering/EntityMaskManager.h"
#include "Rendering/RenderAPI.h"
#include "Rendering/StagingBufferCache.h"
#include "Rendering/Core/API/CommandQueue.h"
#include "Rendering/ImGuiRenderer.h"

#include "Resources/ResourceLoader.h"
#include "Resources/ResourceManager.h"

#include "Audio/AudioAPI.h"

#include "Utilities/RuntimeStats.h"

#include "Game/GameConsole.h"
#include "Game/StateManager.h"
#include "Game/ECS/Systems/Audio/AudioSystem.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Game/ECS/Systems/Rendering/AnimationSystem.h"
#include "Game/ECS/Systems/CameraSystem.h"
#include "Game/ECS/Systems/Physics/PhysicsSystem.h"
#include "Game/ECS/Systems/Physics/TransformApplierSystem.h"
#include "Game/ECS/Systems/Networking/NetworkSystem.h"
#include "Game/Multiplayer/Client/ClientSystem.h"
#include "Game/Multiplayer/Server/ServerSystem.h"
#include "Game/ECS/ComponentOwners/Rendering/MeshPaintComponentOwner.h"


#include "GUI/Core/GUIApplication.h"

#include <imgui/imgui.h>

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
			const Timestamp& delta = g_Clock.GetDeltaTime();
			isRunning = Tick(delta);

			// Fixed update
			accumulator += delta;
			uint32 fixedTickCounter = 0;
			while (accumulator >= g_FixedTimestep)
			{
				fixedClock.Tick();
				FixedTick(g_FixedTimestep);
				accumulator -= g_FixedTimestep;

				//Bailout so we don't get stuck in Fixed Tick
				fixedTickCounter++;
				if (fixedTickCounter > 2)
				{
					accumulator = 0;
					break;
				}
			}
		}
	}

	bool EngineLoop::InitComponentOwners()
	{
		if (!MeshPaintComponentOwner::GetInstance()->Init())
		{
			return false;
		}

		return true;
	}

	bool EngineLoop::InitSystems()
	{
		if (!NetworkSystem::GetInstance().Init())
		{
			return false;
		}

		if (!RenderSystem::GetInstance().Init())
		{
			return false;
		}

		if (!PhysicsSystem::GetInstance()->Init())
		{
			return false;
		}

		if (!AudioSystem::GetInstance().Init())
		{
			return false;
		}

		if (!CameraSystem::GetInstance().Init())
		{
			return false;
		}

		TransformApplierSystem::GetInstance()->Init();
		return true;
	}

	bool EngineLoop::Tick(Timestamp delta)
	{
		// Stats
		RuntimeStats::SetFrameTime((float)delta.AsSeconds());

		// Input
		Input::Tick();

		// Misc
		GameConsole::Get().Tick();

		Thread::Join();

		PlatformNetworkUtils::Tick(delta);

		// Event
		if (!CommonApplication::Get()->Tick())
		{
			return false;
		}

		EventQueue::Tick();

		// Audio
		AudioAPI::Tick();

		// States / ECS-systems
		ClientSystem::StaticTickMainThread(delta);
		ServerSystem::StaticTickMainThread(delta);
		CameraSystem::GetInstance().MainThreadTick(delta);
		StateManager::GetInstance()->Tick(delta);
		AudioSystem::GetInstance().Tick(delta);
		ECSCore::GetInstance()->Tick(delta);

		// Game
		Game::Get().Tick(delta);

		// Rendering
#ifdef LAMBDA_DEVELOPMENT
		// TODO: Move to somewere else, does someone have a suggestion?
		ImGuiRenderer::Get().DrawUI([delta]
			{
				const ImGuiWindowFlags flags =
					ImGuiWindowFlags_NoBackground |
					ImGuiWindowFlags_NoTitleBar |
					ImGuiWindowFlags_NoMove |
					ImGuiWindowFlags_NoResize |
					ImGuiWindowFlags_NoDecoration |
					ImGuiWindowFlags_NoScrollbar |
					ImGuiWindowFlags_NoSavedSettings;

				TSharedRef<Window> mainWindow = CommonApplication::Get()->GetMainWindow();
				const uint32 windowWidth = mainWindow->GetWidth();
				const uint32 size = 250;

				ImGui::SetNextWindowPos(ImVec2((float)(windowWidth - size), 0.0f));
				ImGui::SetNextWindowSize(ImVec2((float32)size, (float32)size));

				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));

				if (ImGui::Begin("BuildInfo", (bool*)(0), flags))
				{
					const GraphicsDeviceDesc& desc = RenderAPI::GetDevice()->GetDesc();
					ImGui::Text("RunInfo:");
					ImGui::Text("FPS: %.2f", 1.0f / delta.AsSeconds());
					ImGui::Spacing();
					ImGui::Text("BuildInfo:");
					ImGui::Text("CrazyCanvas [%s Build]", LAMBDA_CONFIG_NAME);
					ImGui::Text("API: %s", desc.RenderApi.c_str());
					ImGui::Text("Version: %s", desc.ApiVersion.c_str());
					ImGui::Text("Adaper: %s", desc.AdapterName.c_str());
					ImGui::Text("Driver: %s", desc.DriverVersion.c_str());

					// Tells the developer if validation layers are on
					if (!desc.Debug)
					{
						ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
						ImGui::Text("DebugEnabled=false");
						ImGui::PopStyleColor();
					}
					else
					{
						ImGui::Text("DebugEnabled=true");
					}
				}
				ImGui::End();

				ImGui::PopStyleColor();
			});
#endif
		RenderSystem::GetInstance().Render(delta);

		return true;
	}

	void EngineLoop::FixedTick(Timestamp delta)
	{
		Game::Get().FixedTick(delta);
		NetworkUtils::FixedTick(delta);
		StateManager::GetInstance()->FixedTick(delta);
	}

	bool EngineLoop::PreInit(const argh::parser& flagParser)
	{
#ifdef LAMBDA_DEVELOPMENT
		PlatformConsole::Show();

		Log::SetDebuggerOutputEnabled(true);

		Malloc::SetDebugFlags(MEMORY_DEBUG_FLAGS_OVERFLOW_PROTECT | MEMORY_DEBUG_FLAGS_LEAK_CHECK);
#endif

		PlatformTime::PreInit();
		Random::PreInit();

		if (!EngineConfig::LoadFromFile(flagParser))
		{
			return false;
		}

		if (!InputActionSystem::LoadFromFile())
		{
			return false;
		}

		SetFixedTimestep(Timestamp::Seconds(1.0 / EngineConfig::GetDoubleProperty(EConfigOption::CONFIG_OPTION_FIXED_TIMESTEMP)));

		if (!ThreadPool::Init())
		{
			return false;
		}

		if (!CommonApplication::PreInit())
		{
			return false;
		}

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

		if (!EntityMaskManager::Init())
		{
			return false;
		}

		if (!RenderAPI::Init())
		{
			return false;
		}

		if (!AudioAPI::Init())
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

		if (!AnimationSystem::GetInstance().Init())
		{
			return false;
		}

		if (!GUIApplication::Init())
		{
			return false;
		}

		if (!InitComponentOwners())
		{
			return false;
		}

		if (!InitSystems())
		{
			return false;
		}

		if (!StateManager::GetInstance()->Init(ECSCore::GetInstance()))
		{
			return false;
		}

		return true;
	}

	bool EngineLoop::PreRelease()
	{
		RenderAPI::GetGraphicsQueue()->Flush();
		RenderAPI::GetComputeQueue()->Flush();
		RenderAPI::GetCopyQueue()->Flush();

		PlatformNetworkUtils::PreRelease();

		return true;
	}

	bool EngineLoop::Release()
	{
		if (!GameConsole::Get().Release())
		{
			return false;
		}

		EventQueue::Release();

		Input::Release();

		if (!StateManager::GetInstance()->Release())
		{
			return false;
		}

		if (!GUIApplication::Release())
		{
			return false;
		}

		if (!RenderSystem::GetInstance().Release())
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

		if (!StagingBufferCache::Release())
		{
			return false;
		}

		if (!AudioAPI::Release())
		{
			return false;
		}

		EventQueue::UnregisterAll();
		ECSCore::Release();

		if (!ThreadPool::Release())
		{
			return false;
		}

		return true;
	}

	bool EngineLoop::PostRelease()
	{
		ClientSystem::StaticRelease();
		ServerSystem::StaticRelease();
		Thread::Release();
		PlatformNetworkUtils::PostRelease();

		if (!RenderAPI::Release())
		{
			return false;
		}

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
