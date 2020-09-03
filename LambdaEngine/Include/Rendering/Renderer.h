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
	class Window;
	class Texture;
	class RenderGraph;
	class CommandList;
	class TextureView;
	class ImGuiRenderer;
	class GraphicsDevice;
	class CommandAllocator;
	
	struct RendererDesc
	{
		String 			Name			= "";
		bool			Debug			= false;
		Window*			pWindow			= nullptr;
		RenderGraph*	pRenderGraph	= nullptr;
		uint32			BackBufferCount = 3;
	};
	
	class LAMBDA_API Renderer
	{
	public:
		DECL_REMOVE_COPY(Renderer);
		DECL_REMOVE_MOVE(Renderer);

		Renderer(const GraphicsDevice* pGraphicsDevice);
		~Renderer();

		bool Init(const RendererDesc* pDesc);

		void NewFrame(Timestamp delta);
		void PrepareRender(Timestamp delta);

		void Render();

		CommandList* AcquireGraphicsCopyCommandList();
		CommandList* AcquireComputeCopyCommandList();

		FORCEINLINE uint64 GetFrameIndex()		{ return m_FrameIndex; }
		FORCEINLINE uint64 GetModFrameIndex()	{ return m_ModFrameIndex; }
		FORCEINLINE uint32 GetBufferIndex()		{ return m_BackBufferIndex; }

	private:
		bool InitDebugRender();

	private:
		String					m_Name;
		const GraphicsDevice*	m_pGraphicsDevice;

		TSharedRef<SwapChain>	m_SwapChain				= nullptr;
		Texture**				m_ppBackBuffers			= nullptr;
		TextureView**			m_ppBackBufferViews		= nullptr;
		RenderGraph*			m_pRenderGraph			= nullptr;

		uint32					m_BackBufferCount		= 0;

		uint64					m_FrameIndex			= 0;
		uint64					m_ModFrameIndex			= 0;
		uint32					m_BackBufferIndex		= 0;

		TSharedRef<CommandAllocator>	m_CommandAllocators[3];
		TSharedRef<CommandList>			m_CommandLists[3];
		TSharedRef<RenderPass>			m_RenderPass;
		TSharedRef<PipelineState>		m_PipelineState;
		TSharedRef<PipelineLayout>		m_PiplineLayout;
		TSharedRef<Fence>				m_Fence;
	};
}
