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

#include "Network/API/PlatformSocketFactory.h"

#include "Threading/Thread.h"

#include "Resources/ResourceLoader.h"
#include "Resources/ResourceManager.h"

#include "Audio/AudioSystem.h"

#include "Rendering/RenderSystem.h"

namespace LambdaEngine
{
	static void TestRayTracing(IGraphicsDevice* pGraphicsDevice)
	{
		LOG_MESSAGE("\n-------Ray Trace Testing Start-------");

		TopLevelAccelerationStructureDesc topLevelAccelerationStructureDesc = {};
		topLevelAccelerationStructureDesc.pName = "Test TLAS";
		topLevelAccelerationStructureDesc.InitialMaxInstanceCount = 10;

		ITopLevelAccelerationStructure* pTLAS = pGraphicsDevice->CreateTopLevelAccelerationStructure(topLevelAccelerationStructureDesc);

		BottomLevelAccelerationStructureDesc bottomLevelAccelerationStructureDesc = {};
		bottomLevelAccelerationStructureDesc.pName = "Test BLAS";
		bottomLevelAccelerationStructureDesc.MaxTriCount = 12;
		bottomLevelAccelerationStructureDesc.MaxVertCount = 8;
		bottomLevelAccelerationStructureDesc.Updateable = false;

		IBottomLevelAccelerationStructure* pBLAS = pGraphicsDevice->CreateBottomLevelAccelerationStructure(bottomLevelAccelerationStructureDesc);

		SAFERELEASE(pBLAS);
		SAFERELEASE(pTLAS);

		LOG_MESSAGE("-------Ray Trace Testing End-------\n");
	}

	void EngineLoop::Run(Game* pGame)
	{
		Clock clock;
		clock.Reset();

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
		PlatformSocketFactory::Tick(dt);

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
#ifdef LAMBDA_PLATFORM_WINDOWS
        if (!PlatformApplication::PreInit(hInstance))
#else
        if (!PlatformApplication::PreInit())
#endif
        {
            return false;
        }

		PlatformTime::PreInit();
        
        Log::SetDebuggerOutputEnabled(true);
        
#ifndef LAMBDA_PRODUCTION
        PlatformConsole::Show();
#endif
		return true;
	}
	
	bool EngineLoop::Init()
	{
		Thread::Init();

		if (!Input::Init())
		{
			return false;
		}

		if (!PlatformSocketFactory::Init())
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
		PlatformSocketFactory::Release();

#ifndef LAMBDA_PRODUCTION
        PlatformConsole::Close();
#endif
		return true;
	}
}
