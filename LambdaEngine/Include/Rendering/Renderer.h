#pragma once

#include "LambdaEngine.h"

namespace LambdaEngine
{
	class Window;
	class ITexture;
	class ISwapChain;
	class RenderGraph;
	class ITextureView;
	class IGraphicsDevice;
	
	struct RendererDesc
	{
		const char*		pName			= "";
		Window*			pWindow			= nullptr;
		RenderGraph*	pRenderGraph	= nullptr;
	};
	
	class LAMBDA_API Renderer
	{
	public:
		DECL_REMOVE_COPY(Renderer);
		DECL_REMOVE_MOVE(Renderer);

		Renderer(const IGraphicsDevice* pGraphicsDevice);
		~Renderer();

		bool Init(const RendererDesc& desc);

		void Render();

	private:
		const char*				m_pName;
		const IGraphicsDevice*	m_pGraphicsDevice;
		ISwapChain*				m_pSwapChain;
		RenderGraph*			m_pRenderGraph;

		uint32					m_FrameIndex = 0;

		ITexture**				m_ppBackBuffers;
		ITextureView**			m_ppBackBufferViews;
	};
}
