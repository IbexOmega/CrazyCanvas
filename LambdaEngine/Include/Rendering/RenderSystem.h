#pragma once
#include "LambdaEngine.h"

namespace LambdaEngine
{
	class IGraphicsDevice;

	class RenderSystem
	{
	public:
		DECL_STATIC_CLASS(RenderSystem);

		static bool Init();
		static bool Release();

		static void Tick();

		FORCEINLINE static IGraphicsDevice* GetDevice()
		{
			return s_pGraphicsDevice;
		}

	private:
		static IGraphicsDevice* s_pGraphicsDevice;
	};
}