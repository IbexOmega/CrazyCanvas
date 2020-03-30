#include "Engine/EngineLoop.h"

#include "Log/Log.h"

#include "Application/PlatformTime.h"
#include "Application/PlatformMisc.h"
#include "Application/PlatformConsole.h"

#include "Input/API/Input.h"

#include "Rendering/Core/API/IGraphicsDevice.h"

namespace LambdaEngine
{
	void EngineLoop::Run(Game* pGame)
	{
		GraphicsDeviceDesc graphicsDeviceDesc = {};
		graphicsDeviceDesc.Debug = true;

        IGraphicsDevice* pGraphicsDevice = CreateGraphicsDevice(graphicsDeviceDesc, EGraphicsAPI::VULKAN);
        
        bool IsRunning = true;
        while (IsRunning)
        {
            IsRunning = Tick();
            pGame->Tick();
        }

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
