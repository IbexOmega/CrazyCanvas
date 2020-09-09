#pragma once
#include "LambdaEngine.h"

#include "Core/TSharedRef.h"

#include "Time/API/Timestamp.h"

#include "Rendering/Core/API/SwapChain.h"
#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/Fence.h"
#include "Rendering/Core/API/PipelineState.h"
#include "Rendering/Core/API/RenderPass.h"
#include "Rendering/Core/API/PipelineLayout.h"

namespace LambdaEngine
{
	class Scene;
	class Window;
	class Texture;
	class RenderGraph;
	class CommandList;
	class TextureView;
	class ImGuiRenderer;
	class GraphicsDevice;
	class CommandAllocator;
	
	class LAMBDA_API Renderer
	{
	public:
		DECL_REMOVE_COPY(Renderer);
		DECL_REMOVE_MOVE(Renderer);

		static bool Init();
		static bool Release();

		static void SetScene(Scene* pScene);

		static void NewFrame(Timestamp delta);
		static void PrepareRender(Timestamp delta);
		static void Render();

		static CommandList* AcquireGraphicsCopyCommandList();
		static CommandList* AcquireComputeCopyCommandList();

		FORCEINLINE static RenderGraph*		GetRenderGraph()		{ return s_pRenderGraph; }
		FORCEINLINE static uint64			GetFrameIndex()			{ return s_FrameIndex; }
		FORCEINLINE static uint64			GetModFrameIndex()		{ return s_ModFrameIndex; }
		FORCEINLINE static uint32			GetBufferIndex()		{ return s_BackBufferIndex; }

	private:
		static TSharedRef<SwapChain>	s_SwapChain;
		static Texture**				s_ppBackBuffers;
		static TextureView**			s_ppBackBufferViews;
		static RenderGraph*				s_pRenderGraph;
		static Scene*					s_pScene;
		static uint64					s_FrameIndex;
		static uint64					s_ModFrameIndex;
		static uint32					s_BackBufferIndex;
	};
}
