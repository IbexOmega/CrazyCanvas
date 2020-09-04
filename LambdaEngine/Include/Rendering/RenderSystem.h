#pragma once
#include "LambdaEngine.h"

namespace LambdaEngine
{
	class CommandQueue;
	class GraphicsDevice;

	class LAMBDA_API RenderSystem
	{
	public:
		DECL_STATIC_CLASS(RenderSystem);

		static bool Init();
		static bool Release();

		static void Tick();

		FORCEINLINE static GraphicsDevice* GetDevice()
		{
			return s_pGraphicsDevice;
		}

		FORCEINLINE static CommandQueue* GetGraphicsQueue()
		{
			return s_pGraphicsQueue;
		}

		FORCEINLINE static CommandQueue* GetComputeQueue()
		{
			return s_pComputeQueue;
		}

		FORCEINLINE static CommandQueue* GetCopyQueue()
		{
			return s_pCopyQueue;
		}

	private:
		static GraphicsDevice* s_pGraphicsDevice;
		static CommandQueue*	s_pGraphicsQueue;
		static CommandQueue*	s_pComputeQueue;
		static CommandQueue*	s_pCopyQueue;
	};
}