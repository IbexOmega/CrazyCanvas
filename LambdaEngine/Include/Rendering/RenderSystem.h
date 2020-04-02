#pragma once
#include "LambdaEngine.h"

namespace LambdaEngine
{
	class ICommandQueue;
	class IGraphicsDevice;

	class LAMBDA_API RenderSystem
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
		static ICommandQueue*			s_pGraphicsQueue;
		static ICommandQueue*			s_pComputeQueue;
		static ICommandQueue*			s_pCopyQueue;
	};
}