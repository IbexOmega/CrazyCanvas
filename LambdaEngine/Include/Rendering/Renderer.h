#pragma once

#include "LambdaEngine.h"

#include "Time/API/Timestamp.h"

namespace LambdaEngine
{
	class Window;
	class ITexture;
	class ISwapChain;
	class RefactoredRenderGraph;
	class ICommandList;
	class ITextureView;
	class ImGuiRenderer;
	class IGraphicsDevice;
	class ICommandAllocator;
	
	struct RendererDesc
	{
		const char*		pName			= "";
		bool			Debug			= false;
		Window*		pWindow			= nullptr;
		RefactoredRenderGraph*	pRenderGraph	= nullptr;
		uint32			BackBufferCount = 3;
	};
	
	class LAMBDA_API Renderer
	{
	public:
		DECL_REMOVE_COPY(Renderer);
		DECL_REMOVE_MOVE(Renderer);

		Renderer(const IGraphicsDevice* pGraphicsDevice);
		~Renderer();

		bool Init(const RendererDesc* pDesc);

		void NewFrame(Timestamp delta);
		void PrepareRender(Timestamp delta);

		void Render();

		FORCEINLINE uint32 GetFrameIndex()		{ return m_FrameIndex; }
		FORCEINLINE uint32 GetModFrameIndex()	{ return m_ModFrameIndex; }
		FORCEINLINE uint32 GetBufferIndex()		{ return m_BackBufferIndex; }

	private:
		const char*				m_pName;
		const IGraphicsDevice*	m_pGraphicsDevice;

		ISwapChain*				m_pSwapChain					= nullptr;
		RefactoredRenderGraph*			m_pRenderGraph					= nullptr;
		ITexture**				m_ppBackBuffers					= nullptr;
		ITextureView**			m_ppBackBufferViews				= nullptr;

		uint32					m_BackBufferCount				= 0;

		uint32					m_FrameIndex					= 0;
		uint32					m_ModFrameIndex					= 0;
		uint32					m_BackBufferIndex				= 0;

	};
}
