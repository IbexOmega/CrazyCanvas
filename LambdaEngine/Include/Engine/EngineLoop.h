#pragma once
#include "LambdaEngine.h"

#include "Platform/PlatformApplication.h"

namespace LambdaEngine
{
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
		static bool Run();
		static bool Release();
		static bool PostRelease();

	private:
		static void Tick();

		static bool s_IsRunning;
	};
}