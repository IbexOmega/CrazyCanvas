#include "Engine/EngineLoop.h"

#include "Log/Log.h"

#include "Application/PlatformTime.h"
#include "Application/PlatformMisc.h"
#include "Application/PlatformConsole.h"

#include "Input/API/Input.h"

#include "Rendering/Core/API/IGraphicsDevice.h"
#include "Rendering/Core/API/IBuffer.h"
#include "Rendering/Core/API/ITexture.h"
#include "Rendering/Core/API/ITopLevelAccelerationStructure.h"

#include "Network/API/SocketFactory.h"

namespace LambdaEngine
{
	void EngineLoop::Run(Game* pGame)
	{
		LOG_MESSAGE("Oppsi");
		LOG_INFO("Oppsi");
		LOG_WARNING("Oppsi");
		LOG_ERROR("Oppsi");
        LOG_ERROR_CRIT("Oppsi");
        
		GraphicsDeviceDesc graphicsDeviceDesc = {};
		graphicsDeviceDesc.Debug = true;

        IGraphicsDevice* pGraphicsDevice = CreateGraphicsDevice(graphicsDeviceDesc, EGraphicsAPI::VULKAN);
        
		BufferDesc bufferDesc = { };
		bufferDesc.pName			= "VertexBuffer";
		bufferDesc.MemoryType		= EMemoryType::GPU_MEMORY;
		bufferDesc.Flags			= BUFFER_FLAG_UNORDERED_ACCESS_BUFFER | BUFFER_FLAG_COPY_DST;
		bufferDesc.SizeInBytes	= 64;

		IBuffer* pBuffer = pGraphicsDevice->CreateBuffer(bufferDesc);

		TextureDesc textureDesc = { };
		textureDesc.pName		= "Texture";
		textureDesc.Type		= ETextureType::TEXTURE_2D;
		textureDesc.MemoryType	= EMemoryType::GPU_MEMORY;
		textureDesc.Format		= EFormat::R8G8B8A8_UNORM;
		textureDesc.Flags		= TEXTURE_FLAG_COPY_DST | TEXTURE_FLAG_SHADER_RESOURCE;
		textureDesc.Width		= 256;
		textureDesc.Height		= 256;
		textureDesc.Depth		= 1;
		textureDesc.SampleCount		= 1;
		textureDesc.Miplevels	= 1;
		textureDesc.ArrayCount	= 1;

		ITexture* pTexture = pGraphicsDevice->CreateTexture(textureDesc);

		/*TopLevelAccelerationStructureDesc topLevelAccelerationStructureDesc = {};
		topLevelAccelerationStructureDesc.pName = "Test TLAS";

		ITopLevelAccelerationStructure* pTLAS = pGraphicsDevice->CreateTopLevelAccelerationStructure(topLevelAccelerationStructureDesc);*/

        bool IsRunning = true;
        while (IsRunning)
        {
            IsRunning = Tick();
            pGame->Tick();
        }

		SAFERELEASE(pTexture);
		SAFERELEASE(pBuffer);
		/*SAFERELEASE(pTLAS);*/
		SAFERELEASE(pGraphicsDevice);
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

		if (!SocketFactory::Init())
		{
            //TODO: Implement on Mac so that we can return false again
			//return false;
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
