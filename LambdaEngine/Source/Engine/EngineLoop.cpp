#include "Engine/EngineLoop.h"

#include "Log/Log.h"

#include "Application/PlatformTime.h"
#include "Application/PlatformMisc.h"
#include "Application/PlatformConsole.h"

#include "Input/API/Input.h"

#include "Rendering/Core/API/IGraphicsDevice.h"
#include "Rendering/Core/API/IBuffer.h"

namespace LambdaEngine
{
	void EngineLoop::Run(Game* pGame)
	{
		GraphicsDeviceDesc graphicsDeviceDesc = {};
		graphicsDeviceDesc.Debug = true;

        IGraphicsDevice* pGraphicsDevice = CreateGraphicsDevice(graphicsDeviceDesc, EGraphicsAPI::VULKAN);
        
		BufferDesc desc = { };
		desc.pName			= "VertexBuffer";
		desc.MemoryType		= EMemoryType::GPU_MEMORY;
		desc.Flags			= BUFFER_FLAG_UNORDERED_ACCESS_BUFFER | BUFFER_FLAG_COPY_DST;
		desc.SizeInBytes	= 64;

		IBuffer* pBuffer = pGraphicsDevice->CreateBuffer(desc);

        bool IsRunning = true;
        while (IsRunning)
        {
            IsRunning = Tick();
            pGame->Tick();
        }

		pBuffer->Release();
		pGraphicsDevice->Release();
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
