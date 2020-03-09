#pragma once
#include "LambdaEngine.h"

namespace LambdaEngine
{
	class LAMBDA_API EngineLoop
	{
	public:
		DECL_STATIC_CLASS(EngineLoop);

		static bool PreInit();
		static bool Init();
		static bool Run();
		static bool Release();
		static bool PostRelease();

	private:
		static void Tick();
	};
}