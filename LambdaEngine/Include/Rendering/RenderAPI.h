#pragma once
#include "LambdaEngine.h"

#include "Core/TSharedRef.h"
#include "Core/API/GraphicsTypes.h"

namespace LambdaEngine
{
	constexpr const bool IMGUI_ENABLED = true;

	class Renderer;
	class CommandQueue;
	class GraphicsDevice;
	class DeviceChild;

	class LAMBDA_API RenderAPI
	{
	public:
		DECL_STATIC_CLASS(RenderAPI);

		static bool Init();
		static bool Release();

		static void Tick();

		FORCEINLINE static GraphicsDevice* GetDevice()
		{
			return s_pGraphicsDevice;
		}

		FORCEINLINE static CommandQueue* GetGraphicsQueue()
		{
			return s_GraphicsQueue.Get();
		}

		FORCEINLINE static CommandQueue* GetComputeQueue()
		{
			return s_ComputeQueue.Get();
		}

		FORCEINLINE static CommandQueue* GetCopyQueue()
		{
			return s_CopyQueue.Get();
		}

		static void EnqueueResourceRelease(DeviceChild* pResource);

	private:
		static GraphicsDevice* s_pGraphicsDevice;
		static TSharedRef<CommandQueue>	s_GraphicsQueue;
		static TSharedRef<CommandQueue>	s_ComputeQueue;
		static TSharedRef<CommandQueue>	s_CopyQueue;

		inline static SpinLock s_ReleaseResourceLock;
		inline static uint32 s_ModFrameIndex = 0;
		inline static TArray<DeviceChild*> s_DeviceResourcesToRelease[BACK_BUFFER_COUNT];
	};
}