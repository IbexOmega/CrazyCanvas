#pragma once
#include "LambdaEngine.h"

#include "Game/Game.h"

#include "Application/API/PlatformApplication.h"

namespace LambdaEngine
{
	class IGraphicsDevice;

	class LAMBDA_API EngineLoop
	{
	public:
		DECL_STATIC_CLASS(EngineLoop);

#ifdef LAMBDA_PLATFORM_WINDOWS
		static bool PreInit(HINSTANCE hInstance);
#else
		static bool PreInit();
#endif
		static bool Init();
		static void Run(Game* pGame);
		static bool Release();
		static bool PostRelease();

	private:
		static bool Tick();
	};
}
