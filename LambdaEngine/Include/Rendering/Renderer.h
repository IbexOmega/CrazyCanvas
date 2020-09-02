#pragma once

#include "LambdaEngine.h"

#include "Time/API/Timestamp.h"

namespace LambdaEngine
{
	class Window;
	class Texture;
	class SwapChain;
	class RenderGraph;
	class CommandList;
	class TextureView;
	class ImGuiRenderer;
	class GraphicsDevice;
	class CommandAllocator;
	
	struct RendererDesc
	{
		const char*		pName			= "";
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

		ICommandList* AcquireGraphicsCopyCommandList();
		ICommandList* AcquireComputeCopyCommandList();

		FORCEINLINE uint64 GetFrameIndex()		{ return m_FrameIndex; }
		FORCEINLINE uint64 GetModFrameIndex()	{ return m_ModFrameIndex; }
		FORCEINLINE uint32 GetBufferIndex()		{ return m_BackBufferIndex; }

	private:
		const char*				m_pName;
		const GraphicsDevice*	m_pGraphicsDevice;

		SwapChain*				m_pSwapChain					= nullptr;
		RenderGraph*			m_pRenderGraph					= nullptr;
		Texture**				m_ppBackBuffers					= nullptr;
		TextureView**			m_ppBackBufferViews				= nullptr;

		uint32					m_BackBufferCount				= 0;

		uint64					m_FrameIndex					= 0;
		uint64					m_ModFrameIndex					= 0;
		uint32					m_BackBufferIndex				= 0;

	};
}
