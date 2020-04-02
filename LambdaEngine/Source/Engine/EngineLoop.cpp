#include "Engine/EngineLoop.h"

#include "Log/Log.h"

#include "Application/PlatformTime.h"
#include "Application/PlatformMisc.h"
#include "Application/PlatformConsole.h"

#include "Input/API/Input.h"

#include "Rendering/Core/API/IGraphicsDevice.h"
#include "Rendering/Core/API/ITopLevelAccelerationStructure.h"
#include "Rendering/Core/API/IBottomLevelAccelerationStructure.h"

#include "Network/API/PlatformSocketFactory.h"

#include "Threading/Thread.h"

#include "Resources/ResourceHandler.h"

#include "Rendering/RenderSystem.h"

namespace LambdaEngine
{
	void EngineLoop::Run(Game* pGame)
	{
		TestResourceHandler(RenderSystem::GetDevice());
		
        bool IsRunning = true;
        while (IsRunning)
        {
            IsRunning = Tick();
            pGame->Tick();
        }
    }

    bool EngineLoop::Tick()
    {
		Thread::Join();

        if (!PlatformApplication::Tick())
        {
            return false;
        }

        return true;
	}

	void EngineLoop::TestResourceHandler(IGraphicsDevice* pGraphicsDevice)
	{
		ResourceHandler* pResourceHandler = new ResourceHandler(pGraphicsDevice);
		GUID_Lambda failedMeshGUID = pResourceHandler->LoadMeshFromFile("THIS/SHOULD/FAIL.obj");
		GUID_Lambda bunnyMeshGUID = pResourceHandler->LoadMeshFromFile("../Assets/Meshes/bunny.obj");

		SAFEDELETE(pResourceHandler);
	}

	void EngineLoop::TestRayTracing(IGraphicsDevice* pGraphicsDevice)
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

		return true;
	}
	
	bool EngineLoop::Release()
	{
		Input::Release();

		if (!RenderSystem::Release())
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

#ifndef LAMBDA_PRODUCTION
        PlatformConsole::Close();
#endif
		return true;
	}
}
