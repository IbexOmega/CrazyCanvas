#include "Engine/EngineLoop.h"

#include "Log/Log.h"

#include "Application/PlatformTime.h"
#include "Application/PlatformMisc.h"
#include "Application/PlatformConsole.h"

#include "Input/API/Input.h"

#include "Rendering/Core/API/IGraphicsDevice.h"
#include "Rendering/Core/API/IBuffer.h"
#include "Rendering/Core/API/ITexture.h"
#include "Rendering/Core/API/ISwapChain.h"
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
		BufferDesc bufferDesc = { };
		bufferDesc.pName			= "VertexBuffer";
		bufferDesc.MemoryType		= EMemoryType::GPU_MEMORY;
		bufferDesc.Flags			= BUFFER_FLAG_UNORDERED_ACCESS_BUFFER | BUFFER_FLAG_COPY_DST;
		bufferDesc.SizeInBytes		= 64;

		IGraphicsDevice* pDevice = RenderSystem::GetDevice();

		IBuffer* pBuffer = pDevice->CreateBuffer(bufferDesc);
        uint64 bufferAddress = pBuffer->GetDeviceAdress();

		TextureDesc textureDesc = { };
		textureDesc.pName		= "Texture";
		textureDesc.Type		= ETextureType::TEXTURE_2D;
		textureDesc.MemoryType	= EMemoryType::GPU_MEMORY;
		textureDesc.Format		= EFormat::R8G8B8A8_UNORM;
		textureDesc.Flags		= TEXTURE_FLAG_COPY_DST | TEXTURE_FLAG_SHADER_RESOURCE;
		textureDesc.Width		= 256;
		textureDesc.Height		= 256;
		textureDesc.Depth		= 1;
		textureDesc.SampleCount	= 1;
		textureDesc.Miplevels	= 1;
		textureDesc.ArrayCount	= 1;

		ITexture* pTexture = pDevice->CreateTexture(textureDesc);

        SwapChainDesc swapChainDesc = { };
        swapChainDesc.pName         = "Main Window";
        swapChainDesc.BufferCount   = 3;
        swapChainDesc.Format        = EFormat::B8G8R8A8_UNORM;
        swapChainDesc.Width         = 0;
        swapChainDesc.Height        = 0;
        swapChainDesc.SampleCount   = 1;
        
        ISwapChain* pSwapChain = pDevice->CreateSwapChain(PlatformApplication::Get()->GetWindow(), swapChainDesc);

		TestResourceHandler(pDevice);

        bool IsRunning = true;
        while (IsRunning)
        {
            IsRunning = Tick();
            pGame->Tick();
        }

        SAFERELEASE(pSwapChain);
		SAFERELEASE(pTexture);
		SAFERELEASE(pBuffer);
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
